#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <climits>

using namespace std;

// --- 1. Constants & Definitions ---
const int EMPTY = 0;
const int PAWN = 1, KNIGHT = 2, BISHOP = 3, ROOK = 4, QUEEN = 5, KING = 6;
const int WHITE = 8, BLACK = 16;
const int WHITE_KINGSIDE = 1;
const int WHITE_QUEENSIDE = 2;
const int BLACK_KINGSIDE = 4;
const int BLACK_QUEENSIDE = 8;

struct Move {
    int from, to, captured, promoted;
    int score;
    int priorCastlingRights;
    string toString() {
        string files = "abcdefgh";
        string ranks = "12345678";
        string s = "";
        s += files[from % 8];
        s += ranks[from / 8];
        s += files[to % 8];
        s += ranks[to / 8];
        return s;
    }
};

// --- 2. Piece-Square Tables ---
// (Simplified tables to keep code shorter for this test)
const int pawnTable[] = {
    0,  0,  0,  0,  0,  0,  0,  0,
    50, 50, 50, 50, 50, 50, 50, 50,
    10, 10, 20, 30, 30, 20, 10, 10,
     5,  5, 10, 25, 25, 10,  5,  5,
     0,  0,  0, 20, 20,  0,  0,  0,
     5, -5,-10,  0,  0,-10, -5,  5,
     5, 10, 10,-20,-20, 10, 10,  5,
     0,  0,  0,  0,  0,  0,  0,  0
};
const int knightTable[] = {
    -50,-40,-30,-30,-30,-30,-40,-50,
    -40,-20,  0,  0,  0,  0,-20,-40,
    -30,  0, 10, 15, 15, 10,  0,-30,
    -30,  5, 15, 20, 20, 15,  5,-30,
    -30,  0, 15, 20, 20, 15,  0,-30,
    -30,  5, 10, 15, 15, 10,  5,-30,
    -40,-20,  0,  5,  5,  0,-20,-40,
    -50,-40,-30,-30,-30,-30,-40,-50
};

// Bishop: Likes long diagonals and the center
const int bishopTable[] = {
    -20,-10,-10,-10,-10,-10,-10,-20,
    -10,  0,  0,  0,  0,  0,  0,-10,
    -10,  0,  5, 10, 10,  5,  0,-10,
    -10,  5,  5, 10, 10,  5,  5,-10,
    -10,  0, 10, 10, 10, 10,  0,-10,
    -10, 10, 10, 10, 10, 10, 10,-10,
    -10,  5,  0,  0,  0,  0,  5,-10,
    -20,-10,-10,-10,-10,-10,-10,-20
};

// Rook: Likes open files and the 7th rank (to attack pawns)
const int rookTable[] = {
    0,  0,  0,  0,  0,  0,  0,  0,
    5, 10, 10, 10, 10, 10, 10,  5,
   -5,  0,  0,  0,  0,  0,  0, -5,
   -5,  0,  0,  0,  0,  0,  0, -5,
   -5,  0,  0,  0,  0,  0,  0, -5,
   -5,  0,  0,  0,  0,  0,  0, -5,
   -5,  0,  0,  0,  0,  0,  0, -5,
    0,  0,  0,  5,  5,  0,  0,  0
};

// Queen: Similar to Bishop but values mobility more
const int queenTable[] = {
    -20,-10,-10, -5, -5,-10,-10,-20,
    -10,  0,  0,  0,  0,  0,  0,-10,
    -10,  0,  5,  5,  5,  5,  0,-10,
     -5,  0,  5,  5,  5,  5,  0, -5,
      0,  0,  5,  5,  5,  5,  0, -5,
    -10,  5,  5,  5,  5,  5,  0,-10,
    -10,  0,  5,  0,  0,  0,  0,-10,
    -20,-10,-10, -5, -5,-10,-10,-20
};

// King: Wants to hide in the corners (Castled position)
const int kingTable[] = {
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -20,-30,-30,-40,-40,-30,-30,-20,
    -10,-20,-20,-20,-20,-20,-20,-10,
     20, 20,  0,  0,  0,  0, 20, 20,
     20, 30, 10,  0,  0, 10, 30, 20
};

class Engine {
public:
    int board[64];
    bool sideToMove; // true = White

    int castlingRights = 15;

    Engine() { reset(); }

    void reset() {
        for(int i=0; i<64; i++) board[i] = EMPTY;
        int pieces[] = {ROOK, KNIGHT, BISHOP, QUEEN, KING, BISHOP, KNIGHT, ROOK};
        for(int i=0; i<8; i++) {
            board[i] = WHITE | pieces[i];
            board[i+8] = WHITE | PAWN;
            board[i+48] = BLACK | PAWN;
            board[i+56] = BLACK | pieces[i];
        }
        sideToMove = true;
    }

    // --- VISUALIZATION (ASCII GUI) ---
    void printBoard() {
        cout << "\n   +---+---+---+---+---+---+---+---+" << endl;
        for (int r = 7; r >= 0; r--) {
            cout << " " << r + 1 << " |";
            for (int f = 0; f < 8; f++) {
                int sq = r * 8 + f;
                char c = ' ';
                if (board[sq] != EMPTY) {
                    int type = board[sq] & 7;
                    int color = board[sq] & 24;
                    const char* syms = "?pnbrqk";
                    c = syms[type];
                    if (color == WHITE) c = toupper(c);
                }
                cout << " " << c << " |";
            }
            cout << "\n   +---+---+---+---+---+---+---+---+" << endl;
        }
        cout << "     a   b   c   d   e   f   g   h" << endl;
    }

    int evaluate() {
        int score = 0;
        for (int i = 0; i < 64; i++) {
            if (board[i] == EMPTY) continue;
            int type = board[i] & 7;
            bool isWhite = (board[i] & WHITE);
            int idx = isWhite ? (63 - i) : i;
            int val = 0;
            if(type==PAWN) val=100 + pawnTable[idx];
            else if(type==KNIGHT) val=320 + knightTable[idx];
            else if(type==BISHOP) val=330 + bishopTable[idx];
            else if(type==ROOK) val=500 + rookTable[idx];
            else if(type==QUEEN) val=900 + queenTable[idx];
            else if(type==KING) val=20000 + kingTable[idx];

            if (isWhite) score += val; else score -= val;
        }
        return sideToMove ? score : -score;
    }

    bool isSquareAttacked(int sq, int attackingColor) {
        // Simple check (Partial implementation for brevity/safety)
        // Checks Pawns and Knights which are most common early game
        int pawnDir = (attackingColor == WHITE) ? -8 : 8;
        if (sq + pawnDir - 1 >= 0 && (sq % 8) != 0 && board[sq + pawnDir - 1] == (attackingColor | PAWN)) return true;
        if (sq + pawnDir + 1 >= 0 && (sq % 8) != 7 && board[sq + pawnDir + 1] == (attackingColor | PAWN)) return true;

        int kMoves[] = {15, 17, 6, 10, -15, -17, -6, -10};
        for (int m : kMoves) {
            int target = sq + m;
            if (target >= 0 && target < 64 && abs((target%8) - (sq%8)) <= 2)
                if (board[target] == (attackingColor | KNIGHT)) return true;
        }
        return false;
    }

    void generateMoves(vector<Move>& moves) {
        int myColor = sideToMove ? WHITE : BLACK;
        int oppColor = sideToMove ? BLACK : WHITE;

        for (int sq = 0; sq < 64; sq++) {
            if ((board[sq] & myColor) == 0) continue;
            int type = board[sq] & 7;

            // Pawns
            if (type == PAWN) {
                int forward = sideToMove ? 8 : -8;
                int startRank = sideToMove ? 1 : 6;

                int target = sq + forward;
                if (target >= 0 && target < 64 && board[target] == EMPTY) {
                    moves.push_back({sq, target, EMPTY, 0, 0});

                    if ((sq/8) == startRank && board[sq + forward * 2] == EMPTY) {
                        moves.push_back({sq, sq + forward * 2, EMPTY, 0, 0});
                    }
                }

                int offsets[] = {forward - 1, forward + 1};
                for (int offset : offsets) {
                    int capTarget = sq + offset;

                    if (capTarget >= 0 && capTarget < 64) {
                        int currentFile = sq % 8;
                        int targetFile = capTarget % 8;

                        if (abs(currentFile - targetFile) == 1) {
                            if (board[capTarget] & oppColor) {
                                int victimVal = (board[capTarget] & 7) * 10;
                                moves.push_back({sq, capTarget, board[capTarget], 0, 100 + victimVal});
                            }
                        }
                    }
                }
            }

            // Knights/Kings
            else if (type == KNIGHT || type == KING) {
                int offsets[] = {
                    (type == KNIGHT ? 15 : 1), (type == KNIGHT ? 17 : -1),
                    (type == KNIGHT ? 6 : 8), (type == KNIGHT ? 10 : -8),
                    (type == KNIGHT ? -15 : 7), (type == KNIGHT ? -17 : -7),
                    (type == KNIGHT ? -6 : 9), (type == KNIGHT ? -10 : -9)
                };
                int limit = (type == KNIGHT ? 8 : 8);
                for (int i = 0; i < limit; i++) {
                    int target = sq + offsets[i];
                    if (target >= 0 && target < 64) {
                        if (abs((target % 8) - (sq % 8)) > 2) continue;
                        if ((board[target] & myColor) == 0) {
                            int score = (board[target] & oppColor) ? 50 : 0;
                            moves.push_back({sq, target, board[target], 0, score});
                        }
                    }
                }

                if (type == KING) {
                    int myRights = sideToMove ? castlingRights : (castlingRights >> 2);

                    if (myRights & 1) {
                        int f = sq + 1;
                        int g = sq + 2;

                        if (board[f] == EMPTY && board[g] == EMPTY) {
                            if (!isSquareAttacked(sq, oppColor) &&
                                !isSquareAttacked(f, oppColor) &&
                                !isSquareAttacked(g, oppColor)) {
                                moves.push_back({sq, g, EMPTY, 0, 0});
                            }
                        }
                    }

                    if (myRights & 2) {
                        int d = sq - 1;
                        int c = sq - 2;

                        if (board[d] == EMPTY && board[c] == EMPTY) {
                            if (!isSquareAttacked(sq, oppColor) &&
                                !isSquareAttacked(d, oppColor) &&
                                !isSquareAttacked(c, oppColor)) {
                                moves.push_back({sq, c, EMPTY, 0, 0});
                            }
                        }
                    }
                }
            }
            // Sliders (Rook/Bishop/Queen)
            else {
                int start = (type == BISHOP) ? 4 : 0;
                int end = (type == ROOK) ? 4 : 8;
                int dirs[] = {8, -8, 1, -1, 7, -7, 9, -9};
                for (int d = start; d < end; d++) {
                    for (int dist = 1; dist < 8; dist++) {
                        int target = sq + dirs[d] * dist;
                        if (target < 0 || target >= 64) break;
                        int pRank = (target - dirs[d]) / 8, pFile = (target - dirs[d]) % 8;
                        int tRank = target / 8, tFile = target % 8;
                        if (abs(pRank - tRank) > 1 || abs(pFile - tFile) > 1) break;
                        if (board[target] & myColor) break;
                        if (board[target] & oppColor) {
                            moves.push_back({sq, target, board[target], 0, 50});
                            break;
                        }
                        moves.push_back({sq, target, EMPTY, 0, 0});
                        if (board[target] != EMPTY) break;
                    }
                }
            }
        }
        sort(moves.begin(), moves.end(), [](const Move& a, const Move& b) { return a.score > b.score; });
    }

    void makeMove(Move m) {
        m.priorCastlingRights = castlingRights;

        int movedPiece = board[m.from];
        int capturedPiece = board[m.to];

        board[m.to] = board[m.from];
        board[m.from] = EMPTY;

        int type = board[m.to] & 7;
        int color = board[m.to] & 24;

        if (type == KING && abs(m.to - m.from) == 2) {
            int rookFrom, rookTo;
            if (m.to > m.from) {
                rookFrom = m.to + 1; rookTo = m.to - 1;
            } else {
                rookFrom = m.to - 2; rookTo = m.to + 1;
            }

            board[rookTo] = board[rookFrom];
            board[rookFrom] = EMPTY;
        }

        if (type == KING) {
            if (color == WHITE) castlingRights &= ~(WHITE_KINGSIDE | WHITE_QUEENSIDE);
            else castlingRights &= ~(BLACK_KINGSIDE | BLACK_QUEENSIDE);
            // Handle castling rook move
            if (abs(m.to - m.from) == 2) {
                if (m.to == 62) { // White kingside
                    board[61] = board[63];
                    board[63] = EMPTY;
                } else if (m.to == 58) { // White queenside
                    board[59] = board[56];
                    board[56] = EMPTY;
                } else if (m.to == 6) { // Black kingside
                    board[5] = board[7];
                    board[7] = EMPTY;
                } else if (m.to == 2) { // Black queenside
                    board[3] = board[0];
                    board[0] = EMPTY;
                }
            }
        }

        if (m.from == 0 || m.to == 0) castlingRights &= ~BLACK_QUEENSIDE;
        if (m.from == 7 || m.to == 7) castlingRights &= ~BLACK_KINGSIDE;
        if (m.from == 56 || m.to == 56) castlingRights &= ~WHITE_QUEENSIDE;
        if (m.from == 63 || m.to == 63) castlingRights &= ~WHITE_KINGSIDE;

        sideToMove = !sideToMove;
    }

    void unmakeMove(Move m) {
        sideToMove = !sideToMove;
        board[m.from] = board[m.to];
        board[m.to] = m.captured;

        int type = board[m.from] & 7;
        int color = board[m.from] & 24;

        if (type == KING && abs(m.to - m.from) == 2) {
            if (m.to == 62) { // White kingside
                board[63] = board[61];
                board[61] = EMPTY;
            } else if (m.to == 58) { // White queenside
                board[56] = board[59];
                board[59] = EMPTY;
            } else if (m.to == 6) { // Black kingside
                board[7] = board[5];
                board[5] = EMPTY;
            } else if (m.to == 2) { // Black queenside
                board[0] = board[3];
                board[3] = EMPTY;
            }
        }
        castlingRights = m.priorCastlingRights;
    }

    int alphaBeta(int depth, int alpha, int beta) {
        if (depth == 0) return evaluate();
        vector<Move> moves;
        generateMoves(moves);
        int legalMoves = 0;
        for (const auto& move : moves) {
            makeMove(move);
            // Simple legality check: is King attacked?
            int myKing = !sideToMove ? WHITE : BLACK;
            int kPos = -1;
            for(int i=0; i<64; i++) if (board[i] == (myKing | KING)) kPos = i;
            if (isSquareAttacked(kPos, sideToMove ? WHITE : BLACK)) {
                unmakeMove(move);
                continue;
            }
            legalMoves++;
            int score = -alphaBeta(depth - 1, -beta, -alpha);
            unmakeMove(move);
            if (score >= beta) return beta;
            if (score > alpha) alpha = score;
        }
        if (legalMoves == 0) return -20000;
        return alpha;
    }

    Move findBestMove(int depth) {
        vector<Move> moves;
        generateMoves(moves);
        Move bestMove = moves[0];
        int alpha = -INT_MAX;
        int beta = INT_MAX;
        for (const auto& move : moves) {
            makeMove(move);
            int myKing = !sideToMove ? WHITE : BLACK;
            int kPos = -1;
            for(int i=0; i<64; i++) if (board[i] == (myKing | KING)) kPos = i;
            if (isSquareAttacked(kPos, sideToMove ? WHITE : BLACK)) {
                unmakeMove(move);
                continue;
            }
            int score = -alphaBeta(depth - 1, -beta, -alpha);
            unmakeMove(move);
            if (score > alpha) {
                alpha = score;
                bestMove = move;
            }
        }
        return bestMove;
    }
};

// --- 3. NEW MAIN: INTERACTIVE CONSOLE GAME ---
int main() {
    Engine engine;
    string userMove;

    cout << "Welcome to PhylChess!" << endl;
    cout << "You are WHITE (Uppercase pieces)" << endl;
    cout << "Enter moves like 'e2e4' or 'g1f3'." << endl;

    while (true) {
        // 1. Show Board
        engine.printBoard();

        // 2. Get User Move
        cout << "\nYour move: ";
        cin >> userMove;

        if (userMove == "quit") break;
        if (userMove.length() < 4) { cout << "Invalid format!" << endl; continue; }

        // 3. Validate and Apply User Move
        int f = (userMove[0] - 'a') + (userMove[1] - '1') * 8;
        int t = (userMove[2] - 'a') + (userMove[3] - '1') * 8;

        vector<Move> validMoves;
        engine.generateMoves(validMoves);
        bool legal = false;
        Move playerMove;

        for(auto m : validMoves) {
            if(m.from == f && m.to == t) {
                legal = true;
                playerMove = m;
                break;
            }
        }

        if (legal) {
            engine.makeMove(playerMove);
            cout << "You played: " << userMove << endl;

            // 4. Engine Response
            cout << "Engine thinking..." << endl;
            Move engineMove = engine.findBestMove(4); // Search Depth 4
            engine.makeMove(engineMove);
            cout << "Engine played: " << engineMove.toString() << endl;
        } else {
            cout << "Illegal move! Try again." << endl;
        }
    }
    return 0;
}