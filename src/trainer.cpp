struct Datapoint {
    int features[33];
    float target;
};

float sigmoid(float x) {
    return 1 / (1 + exp(-x));
}

void datagen(vector<Datapoint> &data) {
    vector<TtEntry> tt(524288);
    vector<Datapoint> game_data;
    Searcher s;
    Move moves[256];
    uint64_t history[64];
    uint64_t movenums[8];
    int mvcount;
    int v;
    int not_done = 1;
    s.tt_ptr = &tt;
    s.prehistory = history;
    s.abort_time = 1.0 / 0.0;
    while (not_done) {
        retry:
        Board board;
        fread(movenums, sizeof(movenums), 1, RNG_FILE);
        for (int j = 0; j < 8; j++) {
            if (!board.movegen(moves, mvcount)) {
                goto retry;
            }
            board.make_move(moves[movenums[j] % mvcount]);
        }
        s.prehistory_len = 0;
        game_data.clear();
        float outcome;
        for (;;) {
            Move mv(0);
            history[s.prehistory_len++] = board.zobrist;
            for (int depth = 1; depth <= DATAGEN_DEPTH; depth++) {
                v = s.negamax(board, mv, LOST, WON, depth, 0);
            }
            if (v > 20000) {
                outcome = board.stm == WHITE;
                break;
            }
            if (v < -20000) {
                outcome = board.stm != WHITE;
                break;
            }
            if (board.board[mv.to] || (board.board[mv.from] & 7) == PAWN) {
                s.prehistory_len = 0;
            }
            if (s.prehistory_len >= 50 || !mv.from) {
                outcome = 0.5;
                break;
            }
            Datapoint &elem = game_data.emplace_back();
            elem.target = v;
            int flip = board.stm == WHITE ? 0 : FEATURE_FLIP;
            int i = 0;
            for (int sq = A1; sq <= H8; sq++) {
                if (board.board[sq] & 7) {
                    elem.features[i++] = FEATURE[board.board[sq]][sq-A1] ^ flip;
                }
            }
            elem.features[i] = -1;
            board.make_move(mv);
        }
        int flip = 0;
        for (Datapoint &elem : game_data) {
            elem.target = (flip ? 1 - outcome : outcome) * OUTCOME_PART +
                EVAL_PART * sigmoid(elem.target / EVAL_SCALE);
            flip ^= 1;
        }
        MUTEX.lock();
        data.insert(data.end(), game_data.begin(), game_data.end());
        not_done = data.size() < DATAGEN_SIZE;
        printf("datagen: %ld\n", data.size());
        MUTEX.unlock();
    }
}

void optimize(barrier<> &b, vector<Datapoint> &data, int &index, double &total_loss, float lr) {
    while (index < data.size()) {
        b.arrive_and_wait();
        MUTEX.lock();
        int start = min((int) data.size(), index);
        int end = min((int) data.size(), index += BATCH_SIZE / THREADS);
        MUTEX.unlock();

        Nnue grad_acc = {};
        float batch_loss = 0;
        for (int i = start; i < end; i++) {
            Nnue grad = {}; // dv_dparam for output layer, dhidden_dparam for ft
            float dv_dhidden[2][NEURONS];
            float hidden[2][NEURONS];
            memcpy(hidden[0], NNUE.ft_bias, sizeof(NNUE.ft_bias));
            memcpy(hidden[1], NNUE.ft_bias, sizeof(NNUE.ft_bias));
            for (int j = 0; data[i].features[j] != -1; j++) {
                for (int k = 0; k < NEURONS; k++) {
                    hidden[0][k] += NNUE.ft[data[i].features[j]][k];
                    grad.ft[data[i].features[j]][k] += 1; // dhidden_dparam

                    hidden[1][k] += NNUE.ft[data[i].features[j] ^ FEATURE_FLIP][k];
                    grad.ft[data[i].features[j] ^ FEATURE_FLIP][k] += 1; // dhidden_dparam
                }
            }
            float v = NNUE.out_bias;
            for (int i = 0; i < NEURONS; i++) {
                float activated = max(hidden[0][i], 0.f);
                v += NNUE.out[i] * activated;
                grad.out[i] = activated; // dv_dparam = activated
                dv_dhidden[0][i] = NNUE.out[i] * (hidden[0][i] > 0);

                activated = max(hidden[1][i], 0.f);
                v += NNUE.out[i+NEURONS] * activated;
                grad.out[i+NEURONS] = activated; // dv_dparam = activated

                dv_dhidden[1][i] = NNUE.out[i+NEURONS] * (hidden[1][i] > 0);
            }
            float difference = data[i].target - sigmoid(v);
            float loss = difference * difference;

            float dloss_dv = difference * sigmoid(v) * (1 - sigmoid(v));

            grad_acc.out_bias += dloss_dv; // dloss_dparam = dloss_dv * dv_dparam
            for (int i = 0; i < NEURONS_X2; i++) {
                grad_acc.out[i] += dloss_dv * grad.out[i]; // dloss_dparam = dloss_dv * dv_dparam
            }

            float dloss_dhidden[NEURONS];
            for (int i = 0; i < NEURONS; i++) {
                dloss_dhidden[i] = dloss_dv * (dv_dhidden[0][i] + dv_dhidden[1][i]); // dloss_dhidden = dloss_dv * dv_dhidden
                grad_acc.ft_bias[i] += dloss_dhidden[i];
                // dloss_dparam = dloss_dhidden * dhidden_dparam
            }
            for (int j = 0; data[i].features[j] != -1; j++) {
                for (int k = 0; k < NEURONS; k++) {
                    grad_acc.ft[data[i].features[j]][k] += dloss_dhidden[k] * grad.ft[data[i].features[j]][k]; // dloss_dparam = dloss_dhidden * dhidden_dparam
                }
            }

            batch_loss += loss;
        }

        b.arrive_and_wait();

        MUTEX.lock();
        total_loss += batch_loss;

        for (int j = 0; j < 768; j++) {
            for (int k = 0; k < NEURONS; k++) {
                NNUE.ft[j][k] += grad_acc.ft[j][k] * lr;
            }
        }
        for (int k = 0; k < NEURONS; k++) {
            NNUE.ft_bias[k] += grad_acc.ft_bias[k] * lr;
            NNUE.out[k] += grad_acc.out[k] * lr;
            NNUE.out[k+NEURONS] += grad_acc.out[k+NEURONS] * lr;
        }
        NNUE.out_bias += grad_acc.out_bias * lr;
        MUTEX.unlock();
    }
}

void train() {
    int32_t random[772][NEURONS];
    fread(random, sizeof(random), 1, RNG_FILE);
    for (int i = 0; i < 768; i++) {
        if (!i || i == FEATURE_FLIP) {
            continue;
        }
        for (int j = 0; j < NEURONS; j++) {
            NNUE.ft[i][j] = FT_INIT_SCALE * random[i][j] / (float)(1 << 31);
        }
    }
    for (int i = 0; i < NEURONS; i++) {
        NNUE.ft_bias[i] = FT_INIT_SCALE * random[768][i] / (float)(1 << 31);
        NNUE.out[i] = OUT_INIT_SCALE * random[769][i] / (float)(1 << 31);
        NNUE.out[i+NEURONS] = OUT_INIT_SCALE * random[770][i] / (float)(1 << 31);
    }
    NNUE.out_bias = OUT_INIT_SCALE * random[771][0] / (float)(1 << 31);

    float lr = 0.001;
    auto cycle = [&]() {
        barrier<> b(THREADS);
        vector<thread> threads;
        vector<Datapoint> data;
        for (int i = 0; i < THREADS; i++) {
            threads.emplace_back([&]() {
                datagen(data);
            });
        }
        for (auto& t : threads) {
            t.join();
        }

        vector<uint64_t> shuffle(data.size());
        fread(shuffle.data(), sizeof(uint64_t) * data.size(), 1, RNG_FILE);
        for (int i = 0; i < data.size(); i++) {
            swap(data[i], data[shuffle[i] % (data.size() - i) + i]);
        }

        for (int j = 0; j < 500; j++) {
            threads.clear();
            int index = 0;
            double loss = 0;
            for (int i = 0; i < THREADS; i++) {
                threads.emplace_back([&]() {
                    optimize(b, data, index, loss, lr);
                });
            }
            for (auto& t : threads) {
                t.join();
            }
            printf("epoch %d: %g\n", j, loss / data.size());
        }
    };
    cycle();
    lr /= 10;
    cycle();

#ifdef OPENBENCH
    FILE *outfile = fopen("network.txt", "wb");
    fprintf(outfile, "{");
    
    // ft
    fprintf(outfile, "{");
    for (int i = 0; i < 768; i++) {
        fprintf(outfile, "{");
        for (int j = 0; j < NEURONS; j++) {
            fprintf(outfile, "%g,", NNUE.ft[i][j]);
        }
        fprintf(outfile, "},\n");
    }
    fprintf(outfile, "},\n");
    
    // ft_bias
    fprintf(outfile, "{");
    for (int j = 0; j < NEURONS; j++) {
        fprintf(outfile, "%g,", NNUE.ft_bias[j]);
    }
    fprintf(outfile, "},\n");
    
    // out
    fprintf(outfile, "{");
    for (int j = 0; j < NEURONS_X2; j++) {
        fprintf(outfile, "%g,", NNUE.out[j]);
    }
    fprintf(outfile, "},\n");

    // out_bias
    fprintf(outfile, "%g", NNUE.out_bias);

    fprintf(outfile, "}");
    fclose(outfile);
#endif
}
