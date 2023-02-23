#define MAX_HIST 4096

double now() {
    timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec + t.tv_nsec / 1000000000.0;
}

atomic_bool ABORT;
mutex MUTEX;
int FINISHED_DEPTH;
Move BEST_MOVE(0);

struct Searcher {
    uint64_t nodes;
    double abort_time;
    int16_t evals[256];
    int16_t history[2][7][SQUARE_SPAN];
    uint64_t rep_list[256];
    Move killers[256][2];

    int negamax(Board &board, Move &bestmv, int16_t alpha, int16_t beta, int16_t depth, int ply) {
        Move scratch, hashmv(0);
        Move moves[256];
        int score[256];
        int mvcount;

        int pv = beta > alpha+1;
        int in_check = 0;

        TtEntry& slot = TT[board.zobrist % TT.size()];
        uint64_t data = slot.data.load(memory_order_relaxed);
        uint64_t hash_xor_data = slot.hash_xor_data.load(memory_order_relaxed);
        int tt_good = (data ^ board.zobrist) == hash_xor_data;
        TtData tt;
        if (tt_good) {
            memcpy(&tt, &data, sizeof(TtData));

            if (depth > 0 || board.board[tt.mv.to]) {
                hashmv = tt.mv;
            }
            if (depth <= tt.depth && (
                tt.bound == BOUND_EXACT ||
                tt.bound == BOUND_LOWER && tt.eval >= beta ||
                tt.bound == BOUND_UPPER && tt.eval <= alpha
            )) {
                bestmv = tt.mv;
                return tt.eval;
            }
        } else if (depth > 5) {
            depth--;
        }

        evals[ply] = board.eval();
        int eval = tt_good && tt.eval < 20000 && tt.eval > -20000 ? tt.eval : evals[ply];
        int improving = ply > 1 && evals[ply] > evals[ply-2];

        // Reverse Futility Pruning: 16 bytes (bdf2034 vs 98a56ea)
        // 8.0+0.08: 69.60 +- 5.41 (4085 - 2108 - 3807) 4.35 elo/byte
        if (!pv && depth > 0 && depth < 4 && eval >= beta + 75 * depth) {
            return eval;
        }

        // Null Move Pruning: 51 bytes (fef0130 vs 98a56ea)
        // 8.0+0.08: 123.85 +- 5.69 (4993 - 1572 - 3435) 2.43 elo/byte
        if (!pv && eval >= beta && beta > -20000 && depth > 1) {
            Board mkmove = board;
            mkmove.null_move();

            int reduction = (eval - beta) / 128 + depth / 3 + 2;

            int v = -negamax(mkmove, scratch, -beta, -alpha, depth - reduction, ply + 1);
            if (v >= beta) {
                return v;
            }
            in_check = v == LOST;
        }

        if (pv && depth > 0) {
            Board mkmove = board;
            mkmove.null_move();
            in_check = !mkmove.movegen(moves, mvcount);
        }

        // Internal Iterative Deepening: 24 bytes (bd674e0 vs 98a56ea)
        // 8.0+0.08: 67.08 +- 5.38 (4027 - 2120 - 3853) 2.80 elo/byte
        if (depth >= 3 && pv && (!tt_good || tt.bound != BOUND_EXACT)) {
            negamax(board, hashmv, alpha, beta, depth - 2, ply);
        }

        rep_list[ply] = board.zobrist;
        moves[0] = hashmv;
        score[0] = 0;
        mvcount = 1;

        int16_t best = depth > 0 ? LOST + ply : eval;
        if (best >= beta) {
            return best;
        }

        int quiets_to_check_table[] = { 0, 7, 8, 17, 49 };
        int quiets_to_check = depth > 0 && depth < 5 && !pv ? quiets_to_check_table[depth] / (1 + !improving) : -1;

        int raised_alpha = 0;
        int legals = 0;
        for (int i = 0; i < mvcount; i++) {
            if (moves[i].from) {
                int best_i;
                int best_s = -10000;
                for (int j = i; j < mvcount; j++) {
                    int s = score[j] ? score[j] : history
                        [board.stm == BLACK]
                        [board.board[moves[j].from] & 7]
                        [moves[j].to-A1];
                    if (s > best_s) {
                        best_i = j;
                        best_s = s;
                    }
                }
                swap(moves[i], moves[best_i]);
                swap(score[i], score[best_i]);

                int piece = board.board[moves[i].from] & 7;
                int victim = board.board[moves[i].to] & 7;
                int deltas[] = {1350, 210, 390, 440, 680, 1350, 0};

                if (!(quiets_to_check -= !victim)) {
                    break;
                }

                if (depth <= 0 && eval + deltas[victim] <= alpha) {
                    continue;
                }

                Board mkmove = board;
                mkmove.make_move(moves[i]);
                if (!(++nodes & 0xFFF) && (ABORT || now() > abort_time)) {
                    throw 0;
                }

                int is_rep = 0;
                for (int i = ply-1; !is_rep && i >= 0; i -= 2) {
                    is_rep |= rep_list[i] == mkmove.zobrist;
                }
                for (int i = 0; !is_rep && i < PREHISTORY_LENGTH; i++) {
                    is_rep |= PREHISTORY[i] == mkmove.zobrist;
                }

                int16_t v;

                if (is_rep) {
                    v = 0;
                } else if (legals) {
                    // All reductions: 57 bytes (a8e89fa vs 98a56ea)
                    // 8.0+0.08: 181.21 +- 6.27 (6020 - 1231 - 2749) 3.18 elo/byte
                    int reduction = (legals*3 + depth*2) / 32;
                    if (reduction > legals) {
                        reduction = legals;
                    }
                    reduction += legals > 3;
                    reduction -= best_s / 200;
                    if (reduction < 0 || victim || in_check || best_s == 9000) {
                        reduction = 0;
                    }
                    v = -negamax(mkmove, scratch, -alpha-1, -alpha, depth - reduction - 1, ply + 1);
                    if (v > alpha && reduction) {
                        // reduced search failed high, re-search at full depth
                        v = -negamax(mkmove, scratch, -alpha-1, -alpha, depth - 1, ply + 1);
                    }
                    if (v > alpha && v < beta) {
                        // at pv nodes, we need to re-search with full window when move raises alpha
                        // at non-pv nodes, this would be equivalent to the previous search, so skip it
                        v = -negamax(mkmove, scratch, -beta, -alpha, depth - 1, ply + 1);
                    }
                } else {
                    // first legal move is always searched with full window
                    v = -negamax(mkmove, scratch, -beta, -alpha, depth - 1 + in_check, ply + 1);
                }
                if (v == LOST) {
                    moves[i].from = 1;
                } else {
                    legals++;
                }
                if (v > best) {
                    best = v;
                    bestmv = moves[i];
                }
                if (v > alpha) {
                    alpha = v;
                    raised_alpha = 1;
                }
                if (v >= beta) {
                    if (!victim) {
                        for (int j = 0; j < i; j++) {
                            if (board.board[moves[j].to]) {
                                continue;
                            }
                            int16_t& hist = history[board.stm == BLACK][board.board[moves[j].from] & 7][moves[j].to-A1];
                            int change = depth * depth;
                            hist -= change + change * hist / MAX_HIST;
                        }
                        int16_t& hist = history[board.stm == BLACK][board.board[moves[i].from] & 7][moves[i].to-A1];
                        int change = depth * depth;
                        hist += change - change * hist / MAX_HIST;
                        if (!(killers[ply][0] == moves[i])) {
                            killers[ply][1] = killers[ply][0];
                            killers[ply][0] = moves[i];
                        }
                    }
                    break;
                }
            }

            // cases that reach this point with i == 0:
            // 1. hashmv does not exist, moves[0] does not exist => movegen
            // 2. hashmv does not exist, moves[0] exists => already did movegen
            // 3. hashmv exists (implies moves[0] exists) => movegen
            if (!i && (!moves[0].from || hashmv.from)) {
                if (!board.movegen(moves, mvcount, depth > 0)) {
                    return WON;
                }
                for (int j = 0; j < mvcount; j++) {
                    if (hashmv == moves[j]) {
                        swap(moves[0], moves[j]);
                        swap(score[0], score[j]);
                    } else if (board.board[moves[j].to] || moves[j].promo) {
                        // MVV-LVA capture ordering: 3 bytes (78a3963 vs 35f9b66)
                        // 8.0+0.08: 289.03 +- 7.40 (7378 - 563 - 2059) 96.34 elo/byte
                        score[j] = (board.board[moves[j].to] & 7) * 8
                            - (board.board[moves[j].from] & 7)
                            + 10000;
                    } else if (moves[j] == killers[ply][0] || moves[j] == killers[ply][1]) {
                        // Killer move heuristic: 44 bytes (e96d65d vs 35f9b66)
                        // 8.0+0.08: 3.51 +- 5.14 (2906 - 2805 - 4289) 0.08 elo/byte
                        score[j] = 9000;
                    } else {
                        // History heuristic: 90 bytes (d2a7a0e vs 35f9b66)
                        // 8.0+0.08: 225.18 +- 6.42 (6467 - 763 - 2770) 2.50 elo/byte
                        score[j] = 0;
                    }
                }
                // need to step back loop variable in case 1
                i -= !hashmv.from;
            }
        }

        if (depth > 0 && legals == 0) {
            Board mkmove = board;
            mkmove.null_move();
            if (mkmove.movegen(moves, mvcount)) {
                return 0;
            }
        }

        if ((depth > 0 || best != eval) && best > LOST + ply) {
            tt.mv = bestmv;
            tt.eval = best;
            tt.depth = depth > 0 ? depth : 0;
            tt.bound =
                best >= beta ? BOUND_LOWER :
                raised_alpha ? BOUND_EXACT : BOUND_UPPER;
            memcpy(&data, &tt, sizeof(TtData));
            slot.data.store(data, memory_order_relaxed);
            slot.hash_xor_data.store(data ^ board.zobrist, memory_order_relaxed);
        }

        return best;
    }

    void iterative_deepening(double time_alotment, int max_depth=250) {
        memset(history, 0, sizeof(history));
        memset(killers, 0, sizeof(killers));
        nodes = 0;
        abort_time = now() + time_alotment * 0.5;
        time_alotment = now() + time_alotment * 0.03;
        Move mv(0);
        try {
            for (int depth = 1; depth <= max_depth; depth++) {
                int16_t v = negamax(ROOT, mv, LOST, WON, depth, 0);
                MUTEX.lock();
                if (FINISHED_DEPTH < depth) {
                    BEST_MOVE = mv;
                    printf("info depth %d score cp %d pv ", depth, v);
                    mv.put();
                    putchar('\n');
                    FINISHED_DEPTH = depth;
                    if (now() > time_alotment) {
                        depth = max_depth;
                    }
                }
                MUTEX.unlock();
            }
        } catch (...) {}
    }
};
