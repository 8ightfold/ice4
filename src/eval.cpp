// loss: 0.07926    train id: 2024-01-14-09-38-21-frzn-big3

#define S(a, b) (a + (b * 0x10000))

int PIECE_RANK[] = {S(0, 0), S(20, 34), S(28, 39), S(30, 40), S(36, 45), S(45, 57), S(35, 103), S(0, 0), S(104, 119), S(115, 139), S(128, 161), S(133, 172), S(135, 179), S(147, 166), S(123, 154), S(30, 145), S(113, 138), S(123, 140), S(129, 152), S(127, 153), S(125, 154), S(131, 152), S(104, 156), S(72, 166), S(186, 275), S(173, 274), S(175, 278), S(174, 287), S(185, 289), S(205, 287), S(207, 295), S(207, 297), S(325, 508), S(330, 506), S(325, 541), S(316, 566), S(314, 581), S(324, 583), S(312, 594), S(313, 574), S(11, -37), S(3, -1), S(-19, 12), S(-41, 22), S(-32, 31), S(4, 38), S(8, 29), S(23, -29)};
int PIECE_FILE[] = {S(22, 56), S(28, 54), S(35, 48), S(44, 39), S(47, 42), S(52, 46), S(48, 46), S(29, 43), S(107, 145), S(122, 162), S(129, 181), S(136, 184), S(137, 183), S(137, 177), S(128, 166), S(117, 144), S(117, 163), S(125, 167), S(127, 166), S(124, 170), S(127, 167), S(121, 172), S(126, 169), S(121, 160), S(140, 288), S(143, 289), S(153, 288), S(159, 282), S(165, 276), S(155, 281), S(152, 280), S(148, 276), S(339, 537), S(339, 552), S(341, 561), S(342, 568), S(342, 569), S(342, 570), S(348, 563), S(348, 565), S(11, -28), S(17, 2), S(-3, 13), S(-34, 21), S(-23, 17), S(-25, 16), S(16, 1), S(11, -29)};
int MOBILITY[] = {0, S(3, 8), S(1, -2), S(4, 5), S(2, 3), S(3, 2), S(-5, -4)};
int PASSED_PAWN[] = {S(0, 0), S(-4, 3), S(-11, 11), S(-7, 30), S(6, 52), S(12, 91), S(35, 103), S(0, 0)};
int THREATENED[] = {0, S(0, 0), S(-35, -11), S(-30, -30), S(-41, -19), S(-40, -28), S(0, 0)};

#define ISOLATED_PAWN S(-9, -11)
#define DOUBLED_PAWN S(-13, -10)
#define KING_ATTACKS S(14, -10)
#define DOUBLE_KING_ATTACKS S(27, -1)
#define TEMPO S(15, 13)
#define ROOK_BEHIND_PAWN S(-15, -13)
#define BISHOP_PAIR S(22, 47)
