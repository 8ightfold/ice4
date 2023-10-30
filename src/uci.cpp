
#ifdef OPENBENCH
void parse_fen();
int THREADS = 1;
#else
// Replaced for TCEC builds by the minifier.
#define THREADS 1
#endif

void uci() {
    setbuf(stdout, 0);
    char buf[4096], *move;
    int wtime, btime;
#ifdef OPENBENCH
    int opt, value;
#endif
    fgets(buf, 4096, stdin); // uci
    printf(
#ifdef OPENBENCH
        "id name ice4 v3.1\r\n"
        "id author MinusKelvin\n"
        "option name Hash type spin default 8 min 1 max 99999\n"
        "option name Threads type spin default 1 min 1 max 999\n"
        "option name aIIR_DEPTH type spin default 5 min -10000 max 10000\n"
        "option name bRFP_MAX_DEPTH type spin default 4 min -10000 max 10000\n"
        "option name cRFP_MARGIN type spin default 75 min -10000 max 10000\n"
        "option name dRAZOR_MARGIN type spin default 150 min -10000 max 10000\n"
        "option name eNMP_MIN_DEPTH type spin default 1 min -10000 max 10000\n"
        "option name fNMP_MARGIN_DIVISOR type spin default 109 min -10000 max 10000\n"
        "option name gNMP_DEPTH_FACTOR type spin default 375 min -10000 max 10000\n"
        "option name hNMP_REDUCTION type spin default 2 min -10000 max 10000\n"
        "option name iIID_MIN_DEPTH type spin default 3 min -10000 max 10000\n"
        "option name jIID_REDUCTION type spin default 2 min -10000 max 10000\n"
        "option name kLMP_1 type spin default 7 min -10000 max 10000\n"
        "option name lLMP_2 type spin default 8 min -10000 max 10000\n"
        "option name mLMP_3 type spin default 17 min -10000 max 10000\n"
        "option name nLMP_4 type spin default 39 min -10000 max 10000\n"
        "option name oDELTA_PROMO type spin default 1350 min -10000 max 10000\n"
        "option name pDELTA_PAWN type spin default 210 min -10000 max 10000\n"
        "option name qDELTA_KNIGHT type spin default 390 min -10000 max 10000\n"
        "option name rDELTA_BISHOP type spin default 440 min -10000 max 10000\n"
        "option name sDELTA_ROOK type spin default 680 min -10000 max 10000\n"
        "option name tDELTA_QUEEN type spin default 1350 min -10000 max 10000\n"
        "option name uLMR_MOVE type spin default 75 min -10000 max 10000\n"
        "option name vLMR_DEPTH type spin default 100 min -10000 max 10000\n"
        "option name wLMR_EXTRA type spin default 4 min -10000 max 10000\n"
        "option name xLMR_HISTORY type spin default 346 min -10000 max 10000\n"
        "option name yPLAIN_HIST_WEIGHT type spin default 10 min -10000 max 10000\n"
        "option name zCOUNTER_HIST_WEIGHT type spin default 10 min -10000 max 10000\n"
        "option name 0FOLLOWUP_HIST_WEIGHT type spin default 10 min -10000 max 10000\n"
        "option name 1ASPIRATION_BOUND type spin default 10 min -10000 max 10000\n"
        "option name 2HARD_TIME_LIMIT type spin default 10 min -10000 max 10000\n"
        "option name 3SOFT_TIME_LIMIT type spin default 10 min -10000 max 10000\n"
#endif
        "uciok\n"
    );
    for (;;) {
        fgets(buf, 4096, stdin);
        switch (*strtok(buf, " \n")) {
            case 'i': // isready
                printf("readyok\n");
                break;
            case 'q': // quit
                exit(0);
#ifdef OPENBENCH
            case 's': // setoption
                strtok(0, " \n"); // name
                opt = *strtok(0, " \n");
                strtok(0, " \n"); // value
                value = atoi(strtok(0, " \n"));
                switch (opt) {
                    case 'a':
                        IIR_DEPTH = value;
                        break;
                    case 'b':
                        RFP_MAX_DEPTH = value;
                        break;
                    case 'c':
                        RFP_MARGIN = value;
                        break;
                    case 'd':
                        RAZOR_MARGIN = value;
                        break;
                    case 'e':
                        NMP_MIN_DEPTH = value;
                        break;
                    case 'f':
                        NMP_MARGIN_DIVISOR = value;
                        break;
                    case 'g':
                        NMP_DEPTH_FACTOR = value;
                        break;
                    case 'h':
                        NMP_REDUCTION = value;
                        break;
                    case 'i':
                        IID_MIN_DEPTH = value;
                        break;
                    case 'j':
                        IID_REDUCTION = value;
                        break;
                    case 'k':
                        LMP_1 = value;
                        break;
                    case 'l':
                        LMP_2 = value;
                        break;
                    case 'm':
                        LMP_3 = value;
                        break;
                    case 'n':
                        LMP_4 = value;
                        break;
                    case 'o':
                        DELTA_PROMO = value;
                        break;
                    case 'p':
                        DELTA_PAWN = value;
                        break;
                    case 'q':
                        DELTA_KNIGHT = value;
                        break;
                    case 'r':
                        DELTA_BISHOP = value;
                        break;
                    case 's':
                        DELTA_ROOK = value;
                        break;
                    case 't':
                        DELTA_QUEEN = value;
                        break;
                    case 'u':
                        LMR_MOVE = value;
                        break;
                    case 'v':
                        LMR_DEPTH = value;
                        break;
                    case 'w':
                        LMR_EXTRA = value;
                        break;
                    case 'x':
                        LMR_HISTORY = value;
                        break;
                    case 'y':
                        PLAIN_HIST_WEIGHT = value;
                        break;
                    case 'z':
                        COUNTER_HIST_WEIGHT = value;
                        break;
                    case '0':
                        FOLLOWUP_HIST_WEIGHT = value;
                        break;
                    case '1':
                        ASPIRATION_BOUND = value;
                        break;
                    case '2':
                        HARD_TIME_LIMIT = value;
                        break;
                    case '3':
                        SOFT_TIME_LIMIT = value;
                        break;
                    case 'H':
                        TT = vector<TtEntry>(value * 65536);
                        break;
                    case 'T':
                        THREADS = value;
                        break;
                }
                break;
#endif
            case 'p': // position
                ROOT = Board();
#ifdef OPENBENCH
                if (!strcmp(strtok(0, " \n"), "fen")) {
                    parse_fen();
                }
#else
                strtok(0, " \n"); // startpos
#endif
                strtok(0, " \n"); // moves
                PREHISTORY_LENGTH = 0;
                while (move = strtok(0, " \n")) {
                    PREHISTORY[PREHISTORY_LENGTH++] = ROOT.zobrist;
                    Move mv(
                        move[1] * 10 + move[0] - 566,
                        move[3] * 10 + move[2] - 566,
                        // maps promotion chars to piece enums
                        //       'q'    'r'    'b'    'n'    '\0' 
                        // cast  113    114    98     110    0
                        // % 53  7      8      45     4      0
                        // * 5   35     40     225    20     0
                        // % 6   5      4      3      2      0
                        // enum  QUEEN  ROOK   BISHOP KNIGHT EMPTY
                        move[4] % 53 * 5 % 6
                    );
                    if ((ROOT.board[mv.from] & 7) == PAWN || ROOT.board[mv.to]) {
                        PREHISTORY_LENGTH = 0;
                    }
                    ROOT.make_move(mv);
                }
                break;
            case 'g': // go
#ifdef OPENBENCH
                char *w;
                wtime = 1 << 30;
                btime = 1 << 30;
                while (w = strtok(0, " \n")) {
                    if (!strcmp(w, "wtime")) {
                        wtime = atoi(strtok(0, " \n"));
                    } else if (!strcmp(w, "btime")) {
                        btime = atoi(strtok(0, " \n"));
                    }
                }
#else
                strtok(0, " \n"); // wtime
                wtime = atoi(strtok(0, " \n"));
                strtok(0, " \n"); // btime
                btime = atoi(strtok(0, " \n"));
#endif
                double time_alotment = (ROOT.stm == WHITE ? wtime : btime) / 1e3;
                ABORT = 0;
                FINISHED_DEPTH = 0;
                vector<thread> threads;
                for (int i = 0; i < THREADS; i++) {
                    threads.emplace_back([time_alotment]() {
                        Searcher s;
                        s.iterative_deepening(time_alotment);
                        ABORT = 1;
                    });
                }
#ifdef OPENBENCH
                if (wtime == 1 << 30) {
                    while (1) {
                        fgets(buf, 4096, stdin);
                        if (buf[0] == 's') {
                            // stop
                            break;
                        } else if (buf[1] == 'q') {
                            // quit
                            ABORT = 1;
                            for (auto& t : threads) {
                                t.join();
                            }
                            return;
                        }
                    }
                    ABORT = 1;
                }
#endif
                for (int i = 0; i < THREADS; i++) {
                    threads[i].join();
                }
                printf("bestmove ");
                BEST_MOVE.put_with_newline();
        }
    }
}

#ifdef OPENBENCH
void parse_fen() {
    int rank = 7;
    int file = 0;
    char *word = strtok(0, " \n");
    for (char c = *word++; c; c = *word++) {
        int sq = (rank * 10) + file + A1;
        file++;
        switch (c) {
            case 'P':
                ROOT.edit(sq, WHITE | PAWN);
                break;
            case 'N':
                ROOT.edit(sq, WHITE | KNIGHT);
                break;
            case 'B':
                ROOT.edit(sq, WHITE | BISHOP);
                break;
            case 'R':
                ROOT.edit(sq, WHITE | ROOK);
                break;
            case 'Q':
                ROOT.edit(sq, WHITE | QUEEN);
                break;
            case 'K':
                ROOT.edit(sq, WHITE | KING);
                break;
            case 'p':
                ROOT.edit(sq, BLACK | PAWN);
                break;
            case 'n':
                ROOT.edit(sq, BLACK | KNIGHT);
                break;
            case 'b':
                ROOT.edit(sq, BLACK | BISHOP);
                break;
            case 'r':
                ROOT.edit(sq, BLACK | ROOK);
                break;
            case 'q':
                ROOT.edit(sq, BLACK | QUEEN);
                break;
            case 'k':
                ROOT.edit(sq, BLACK | KING);
                break;
            case '/':
                file = 0;
                rank--;
                break;
            default:
                file += c - '1';
                for (int i = 0; i < c - '0'; i++) {
                    ROOT.edit(sq+i, 0);
                }
                break;
        }
    }

    if (*strtok(0, " \n") == 'b') {
        ROOT.stm = BLACK;
        ROOT.zobrist ^= ZOBRIST.stm;
    }

    word = strtok(0, " \n");
    int remove_white_short = 1;
    int remove_white_long = 1;
    int remove_black_short = 1;
    int remove_black_long = 1;
    for (char c = *word++; c; c = *word++) {
        switch (c) {
            case 'K':
                remove_white_short = 0;
                break;
            case 'Q':
                remove_white_long = 0;
                break;
            case 'k':
                remove_black_short = 0;
                break;
            case 'q':
                remove_black_long = 0;
                break;
        }
    }
    if (remove_white_short) ROOT.remove_castle_rights(0, SHORT_CASTLE);
    if (remove_white_long) ROOT.remove_castle_rights(0, LONG_CASTLE);
    if (remove_black_short) ROOT.remove_castle_rights(1, SHORT_CASTLE);
    if (remove_black_long) ROOT.remove_castle_rights(1, LONG_CASTLE);

    word = strtok(0, " \n");
    if (*word != '-') {
        ROOT.ep_square = (word[1] - '1') * 10 + word[0] - 'a' + A1;
    }

    strtok(0, " \n"); // halfmove clock

    strtok(0, " \n"); // fullmove counter
}
#endif
