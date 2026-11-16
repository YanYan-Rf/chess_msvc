#include "ChessEngine.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace {

constexpr int PIECE_VALUES[7] = {
    0,     // None
    100,   // Pawn
    500,   // Rook
    320,   // Knight
    330,   // Bishop
    900,   // Queen
    20000  // King
};

// 从白方视角定义的棋子位置价值表（row 0 为第8横线，row 7 为第1横线）
constexpr int PAWN_PST[8][8] = {
    { 0,  0,  0,  0,  0,  0,  0,  0},
    {50, 50, 50, 50, 50, 50, 50, 50},
    {10, 10, 20, 30, 30, 20, 10, 10},
    { 5,  5, 10, 25, 25, 10,  5,  5},
    { 0,  0,  0, 22, 22,  0,  0,  0},
    { 5, -5,-10,  0,  0,-10, -5,  5},
    { 5, 10, 10,-20,-20, 10, 10,  5},
    { 0,  0,  0,  0,  0,  0,  0,  0}
};

constexpr int KNIGHT_PST[8][8] = {
    {-50,-40,-30,-30,-30,-30,-40,-50},
    {-40,-20,  0,  0,  0,  0,-20,-40},
    {-30,  0, 10, 15, 15, 10,  0,-30},
    {-30,  5, 15, 20, 20, 15,  5,-30},
    {-30,  0, 15, 20, 20, 15,  0,-30},
    {-30,  5, 10, 15, 15, 10,  5,-30},
    {-40,-20,  0,  5,  5,  0,-20,-40},
    {-50,-40,-30,-30,-30,-30,-40,-50}
};

constexpr int BISHOP_PST[8][8] = {
    {-20,-10,-10,-10,-10,-10,-10,-20},
    {-10,  0,  0,  0,  0,  0,  0,-10},
    {-10,  0,  5, 10, 10,  5,  0,-10},
    {-10,  5,  5, 10, 10,  5,  5,-10},
    {-10,  0, 10, 10, 10, 10,  0,-10},
    {-10, 10, 10, 10, 10, 10, 10,-10},
    {-10,  5,  0,  0,  0,  0,  5,-10},
    {-20,-10,-10,-10,-10,-10,-10,-20}
};

constexpr int ROOK_PST[8][8] = {
    { 0,  0,  0,  0,  0,  0,  0,  0},
    { 5, 10, 10, 10, 10, 10, 10,  5},
    {-5,  0,  0,  0,  0,  0,  0, -5},
    {-5,  0,  0,  0,  0,  0,  0, -5},
    {-5,  0,  0,  0,  0,  0,  0, -5},
    {-5,  0,  0,  0,  0,  0,  0, -5},
    {-5,  0,  0,  0,  0,  0,  0, -5},
    { 0,  0,  0,  5,  5,  0,  0,  0}
};

constexpr int QUEEN_PST[8][8] = {
    {-20,-10,-10, -5, -5,-10,-10,-20},
    {-10,  0,  0,  0,  0,  0,  0,-10},
    {-10,  0,  5,  5,  5,  5,  0,-10},
    { -5,  0,  5,  5,  5,  5,  0, -5},
    {  0,  0,  5,  5,  5,  5,  0, -5},
    {-10,  5,  5,  5,  5,  5,  0,-10},
    {-10,  0,  5,  0,  0,  0,  0,-10},
    {-20,-10,-10, -5, -5,-10,-10,-20}
};

constexpr int KING_PST[8][8] = {
    {-30,-40,-40,-50,-50,-40,-40,-30},
    {-30,-40,-40,-50,-50,-40,-40,-30},
    {-30,-40,-40,-50,-50,-40,-40,-30},
    {-30,-40,-40,-50,-50,-40,-40,-30},
    {-20,-30,-30,-40,-40,-30,-30,-20},
    {-10,-20,-20,-20,-20,-20,-20,-10},
    { 20, 20,  0,  0,  0,  0, 20, 20},
    { 20, 30, 10,  0,  0, 10, 30, 20}
};

char PieceLetter(PieceType type) {
    switch (type) {
        case PieceType::Rook:   return 'R';
        case PieceType::Knight: return 'N';
        case PieceType::Bishop: return 'B';
        case PieceType::Queen:  return 'Q';
        case PieceType::King:   return 'K';
        default: return '\0';
    }
}

std::string SquareName(Pos p) {
    std::string s;
    s += static_cast<char>('a' + p.col);
    s += static_cast<char>('8' - p.row);
    return s;
}

} // namespace

BoardState ChessEngine::CreateInitialState() {
    BoardState s;
    constexpr PieceType backRank[8] = {
        PieceType::Rook, PieceType::Knight, PieceType::Bishop, PieceType::Queen,
        PieceType::King, PieceType::Bishop, PieceType::Knight, PieceType::Rook
    };

    for (int c = 0; c < 8; ++c) {
        s.board[0][c] = {backRank[c], PieceColor::Black};
        s.board[1][c] = {PieceType::Pawn, PieceColor::Black};
        s.board[6][c] = {PieceType::Pawn, PieceColor::White};
        s.board[7][c] = {backRank[c], PieceColor::White};
    }

    s.turn = PieceColor::White;
    s.whiteCastleK = s.whiteCastleQ = true;
    s.blackCastleK = s.blackCastleQ = true;
    s.enPassantTarget = {-1, -1};
    s.halfMoveClock = 0;
    return s;
}

PieceColor ChessEngine::Opposite(PieceColor c) {
    if (c == PieceColor::White) return PieceColor::Black;
    if (c == PieceColor::Black) return PieceColor::White;
    return PieceColor::None;
}

Pos ChessEngine::FindKing(const BoardState& state, PieceColor color) {
    for (int r = 0; r < 8; ++r) {
        for (int c = 0; c < 8; ++c) {
            if (state.board[r][c].type == PieceType::King &&
                state.board[r][c].color == color) {
                return {r, c};
            }
        }
    }
    return {-1, -1};
}

bool ChessEngine::IsSquareAttacked(const BoardState& state, Pos sq, PieceColor byColor) {
    // 1. 兵攻击检测
    int pawnSourceRow = (byColor == PieceColor::White) ? sq.row + 1 : sq.row - 1;
    if (pawnSourceRow >= 0 && pawnSourceRow < 8) {
        for (int dc : {-1, 1}) {
            int pc = sq.col + dc;
            if (pc >= 0 && pc < 8) {
                const Piece& p = state.board[pawnSourceRow][pc];
                if (p.color == byColor && p.type == PieceType::Pawn) {
                    return true;
                }
            }
        }
    }

    // 2. 马攻击检测
    constexpr int knightDirs[8][2] = {
        {-2, -1}, {-2, 1}, {-1, -2}, {-1, 2},
        { 1, -2}, { 1, 2}, { 2, -1}, { 2, 1}
    };
    for (const auto& d : knightDirs) {
        int r = sq.row + d[0];
        int c = sq.col + d[1];
        if (r >= 0 && r < 8 && c >= 0 && c < 8) {
            const Piece& p = state.board[r][c];
            if (p.color == byColor && p.type == PieceType::Knight) {
                return true;
            }
        }
    }

    // 3. 王相邻攻击检测
    for (int dr = -1; dr <= 1; ++dr) {
        for (int dc = -1; dc <= 1; ++dc) {
            if (dr == 0 && dc == 0) continue;
            int r = sq.row + dr;
            int c = sq.col + dc;
            if (r >= 0 && r < 8 && c >= 0 && c < 8) {
                const Piece& p = state.board[r][c];
                if (p.color == byColor && p.type == PieceType::King) {
                    return true;
                }
            }
        }
    }

    // 4. 斜线（象 / 后）
    constexpr int diagDirs[4][2] = {{-1, -1}, {-1, 1}, {1, -1}, {1, 1}};
    for (const auto& d : diagDirs) {
        int r = sq.row + d[0];
        int c = sq.col + d[1];
        while (r >= 0 && r < 8 && c >= 0 && c < 8) {
            const Piece& p = state.board[r][c];
            if (!p.IsEmpty()) {
                if (p.color == byColor &&
                    (p.type == PieceType::Bishop || p.type == PieceType::Queen)) {
                    return true;
                }
                break;
            }
            r += d[0];
            c += d[1];
        }
    }

    // 5. 直线（车 / 后）
    constexpr int orthoDirs[4][2] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};
    for (const auto& d : orthoDirs) {
        int r = sq.row + d[0];
        int c = sq.col + d[1];
        while (r >= 0 && r < 8 && c >= 0 && c < 8) {
            const Piece& p = state.board[r][c];
            if (!p.IsEmpty()) {
                if (p.color == byColor &&
                    (p.type == PieceType::Rook || p.type == PieceType::Queen)) {
                    return true;
                }
                break;
            }
            r += d[0];
            c += d[1];
        }
    }

    return false;
}

bool ChessEngine::IsInCheck(const BoardState& state, PieceColor color) {
    Pos kingPos = FindKing(state, color);
    if (!kingPos.IsValid()) return false;
    return IsSquareAttacked(state, kingPos, Opposite(color));
}

std::vector<Move> ChessEngine::GeneratePseudoLegalMoves(const BoardState& state) {
    std::vector<Move> moves;
    moves.reserve(48);

    PieceColor us = state.turn;
    PieceColor them = Opposite(us);

    auto addPawnMove = [&](Pos from, Pos to) {
        int promoRow = (us == PieceColor::White) ? 0 : 7;
        if (to.row == promoRow) {
            for (PieceType pt : {PieceType::Queen, PieceType::Rook, PieceType::Bishop, PieceType::Knight}) {
                moves.push_back({from, to, pt, false, false});
            }
        } else {
            moves.push_back({from, to, PieceType::None, false, false});
        }
    };

    for (int r = 0; r < 8; ++r) {
        for (int c = 0; c < 8; ++c) {
            const Piece& p = state.board[r][c];
            if (p.color != us) continue;

            Pos from{r, c};
            switch (p.type) {
                case PieceType::Pawn: {
                    int dir = (us == PieceColor::White) ? -1 : 1;
                    int startRow = (us == PieceColor::White) ? 6 : 1;

                    int nr = r + dir;
                    if (nr >= 0 && nr < 8 && state.board[nr][c].IsEmpty()) {
                        addPawnMove(from, {nr, c});
                        int nr2 = r + dir * 2;
                        if (r == startRow && state.board[nr2][c].IsEmpty()) {
                            moves.push_back({from, {nr2, c}, PieceType::None, false, false});
                        }
                    }

                    for (int dc : {-1, 1}) {
                        int nc = c + dc;
                        if (nr >= 0 && nr < 8 && nc >= 0 && nc < 8) {
                            if (state.board[nr][nc].color == them) {
                                addPawnMove(from, {nr, nc});
                            } else if (state.enPassantTarget.row == nr && state.enPassantTarget.col == nc) {
                                moves.push_back({from, {nr, nc}, PieceType::None, true, false});
                            }
                        }
                    }
                    break;
                }

                case PieceType::Knight: {
                    constexpr int dirs[8][2] = {
                        {-2, -1}, {-2, 1}, {-1, -2}, {-1, 2},
                        { 1, -2}, { 1, 2}, { 2, -1}, { 2, 1}
                    };
                    for (const auto& d : dirs) {
                        int nr = r + d[0];
                        int nc = c + d[1];
                        if (nr >= 0 && nr < 8 && nc >= 0 && nc < 8 && state.board[nr][nc].color != us) {
                            moves.push_back({from, {nr, nc}, PieceType::None, false, false});
                        }
                    }
                    break;
                }

                case PieceType::Bishop:
                case PieceType::Rook:
                case PieceType::Queen: {
                    constexpr int allDirs[8][2] = {
                        {-1, 0}, {1, 0}, {0, -1}, {0, 1},
                        {-1, -1}, {-1, 1}, {1, -1}, {1, 1}
                    };
                    int startIdx = (p.type == PieceType::Bishop) ? 4 : 0;
                    int endIdx = (p.type == PieceType::Rook) ? 4 : 8;
                    for (int i = startIdx; i < endIdx; ++i) {
                        int nr = r + allDirs[i][0];
                        int nc = c + allDirs[i][1];
                        while (nr >= 0 && nr < 8 && nc >= 0 && nc < 8) {
                            if (state.board[nr][nc].IsEmpty()) {
                                moves.push_back({from, {nr, nc}, PieceType::None, false, false});
                            } else {
                                if (state.board[nr][nc].color == them) {
                                    moves.push_back({from, {nr, nc}, PieceType::None, false, false});
                                }
                                break;
                            }
                            nr += allDirs[i][0];
                            nc += allDirs[i][1];
                        }
                    }
                    break;
                }

                case PieceType::King: {
                    for (int dr = -1; dr <= 1; ++dr) {
                        for (int dc = -1; dc <= 1; ++dc) {
                            if (dr == 0 && dc == 0) continue;
                            int nr = r + dr;
                            int nc = c + dc;
                            if (nr >= 0 && nr < 8 && nc >= 0 && nc < 8 && state.board[nr][nc].color != us) {
                                moves.push_back({from, {nr, nc}, PieceType::None, false, false});
                            }
                        }
                    }

                    // 王车易位
                    int homeRow = (us == PieceColor::White) ? 7 : 0;
                    if (r == homeRow && c == 4 && !IsSquareAttacked(state, from, them)) {
                        bool canK = (us == PieceColor::White) ? state.whiteCastleK : state.blackCastleK;
                        bool canQ = (us == PieceColor::White) ? state.whiteCastleQ : state.blackCastleQ;

                        if (canK &&
                            state.board[homeRow][5].IsEmpty() &&
                            state.board[homeRow][6].IsEmpty() &&
                            state.board[homeRow][7].type == PieceType::Rook &&
                            state.board[homeRow][7].color == us &&
                            !IsSquareAttacked(state, {homeRow, 5}, them) &&
                            !IsSquareAttacked(state, {homeRow, 6}, them)) {
                            moves.push_back({from, {homeRow, 6}, PieceType::None, false, true});
                        }

                        if (canQ &&
                            state.board[homeRow][3].IsEmpty() &&
                            state.board[homeRow][2].IsEmpty() &&
                            state.board[homeRow][1].IsEmpty() &&
                            state.board[homeRow][0].type == PieceType::Rook &&
                            state.board[homeRow][0].color == us &&
                            !IsSquareAttacked(state, {homeRow, 3}, them) &&
                            !IsSquareAttacked(state, {homeRow, 2}, them)) {
                            moves.push_back({from, {homeRow, 2}, PieceType::None, false, true});
                        }
                    }
                    break;
                }

                default:
                    break;
            }
        }
    }

    return moves;
}

void ChessEngine::MakeMove(BoardState& state, const Move& move) {
    Piece moving = state.board[move.from.row][move.from.col];
    bool isCapture = !state.board[move.to.row][move.to.col].IsEmpty() || move.isEnPassant;

    if (moving.type == PieceType::Pawn || isCapture) {
        state.halfMoveClock = 0;
    } else {
        ++state.halfMoveClock;
    }

    // 移动棋子
    state.board[move.to.row][move.to.col] = moving;
    state.board[move.from.row][move.from.col] = {};

    // 吃过路兵时清除被吃掉的兵
    if (move.isEnPassant) {
        state.board[move.from.row][move.to.col] = {};
    }

    // 王车易位时同步移动车
    if (move.isCastle) {
        int row = move.to.row;
        if (move.to.col == 6) {
            state.board[row][5] = state.board[row][7];
            state.board[row][7] = {};
        } else if (move.to.col == 2) {
            state.board[row][3] = state.board[row][0];
            state.board[row][0] = {};
        }
    }

    // 升变
    if (move.promotion != PieceType::None) {
        state.board[move.to.row][move.to.col].type = move.promotion;
    }

    // 更新过路兵目标格
    if (moving.type == PieceType::Pawn && std::abs(move.to.row - move.from.row) == 2) {
        state.enPassantTarget = {(move.from.row + move.to.row) / 2, move.from.col};
    } else {
        state.enPassantTarget = {-1, -1};
    }

    // 更新王车易位权限
    if (moving.type == PieceType::King) {
        if (moving.color == PieceColor::White) {
            state.whiteCastleK = state.whiteCastleQ = false;
        } else {
            state.blackCastleK = state.blackCastleQ = false;
        }
    }
    auto clearRookRight = [&](Pos p) {
        if (p.row == 7 && p.col == 0) state.whiteCastleQ = false;
        if (p.row == 7 && p.col == 7) state.whiteCastleK = false;
        if (p.row == 0 && p.col == 0) state.blackCastleQ = false;
        if (p.row == 0 && p.col == 7) state.blackCastleK = false;
    };
    clearRookRight(move.from);
    clearRookRight(move.to);

    state.turn = Opposite(state.turn);
}

std::vector<Move> ChessEngine::GenerateLegalMoves(const BoardState& state) {
    std::vector<Move> pseudo = GeneratePseudoLegalMoves(state);
    std::vector<Move> legal;
    legal.reserve(pseudo.size());

    PieceColor us = state.turn;
    for (const Move& m : pseudo) {
        BoardState next = state;
        MakeMove(next, m);
        if (!IsInCheck(next, us)) {
            legal.push_back(m);
        }
    }
    return legal;
}

GameResult ChessEngine::EvaluateGameResult(const BoardState& state, const std::vector<Move>& legalMoves) {
    if (legalMoves.empty()) {
        if (IsInCheck(state, state.turn)) {
            return (state.turn == PieceColor::White) ? GameResult::BlackWin : GameResult::WhiteWin;
        }
        return GameResult::Stalemate;
    }

    if (state.halfMoveClock >= 100) {
        return GameResult::DrawFiftyMove;
    }

    // 简单子力不足判定（仅剩双王，或王+轻子 vs 王）
    int whiteMinors = 0;
    int blackMinors = 0;
    for (int r = 0; r < 8; ++r) {
        for (int c = 0; c < 8; ++c) {
            const Piece& p = state.board[r][c];
            if (p.IsEmpty() || p.type == PieceType::King) continue;
            if (p.type == PieceType::Pawn || p.type == PieceType::Rook || p.type == PieceType::Queen) {
                return GameResult::Ongoing;
            }
            if (p.color == PieceColor::White) ++whiteMinors;
            else ++blackMinors;
        }
    }
    if (whiteMinors <= 1 && blackMinors <= 1 && (whiteMinors + blackMinors) <= 1) {
        return GameResult::DrawInsufficient;
    }

    return GameResult::Ongoing;
}

std::string ChessEngine::MoveToNotation(const BoardState& beforeState, const Move& move, const BoardState& afterState) {
    std::string note;
    if (move.isCastle) {
        note = (move.to.col == 6) ? "O-O" : "O-O-O";
    } else {
        Piece moving = beforeState.board[move.from.row][move.from.col];
        bool capture = !beforeState.board[move.to.row][move.to.col].IsEmpty() || move.isEnPassant;

        if (moving.type == PieceType::Pawn) {
            if (capture) {
                note += static_cast<char>('a' + move.from.col);
                note += 'x';
            }
            note += SquareName(move.to);
        } else {
            note += PieceLetter(moving.type);
            if (capture) note += 'x';
            note += SquareName(move.to);
        }

        if (move.promotion != PieceType::None) {
            note += '=';
            note += PieceLetter(move.promotion);
        }
    }

    std::vector<Move> nextLegal = GenerateLegalMoves(afterState);
    if (IsInCheck(afterState, afterState.turn)) {
        note += nextLegal.empty() ? '#' : '+';
    }
    return note;
}

int ChessEngine::EvaluateBoard(const BoardState& state) {
    int whiteScore = 0;
    int blackScore = 0;

    for (int r = 0; r < 8; ++r) {
        for (int c = 0; c < 8; ++c) {
            const Piece& p = state.board[r][c];
            if (p.IsEmpty()) continue;

            int pstRow = (p.color == PieceColor::White) ? r : (7 - r);
            int val = PIECE_VALUES[static_cast<int>(p.type)];

            switch (p.type) {
                case PieceType::Pawn:   val += PAWN_PST[pstRow][c]; break;
                case PieceType::Knight: val += KNIGHT_PST[pstRow][c]; break;
                case PieceType::Bishop: val += BISHOP_PST[pstRow][c]; break;
                case PieceType::Rook:   val += ROOK_PST[pstRow][c]; break;
                case PieceType::Queen:  val += QUEEN_PST[pstRow][c]; break;
                case PieceType::King:   val += KING_PST[pstRow][c]; break;
                default: break;
            }

            if (p.color == PieceColor::White) whiteScore += val;
            else blackScore += val;
        }
    }

    int diff = whiteScore - blackScore;
    return (state.turn == PieceColor::White) ? diff : -diff;
}

int ChessEngine::MoveOrderScore(const BoardState& state, const Move& m) {
    int score = 0;
    const Piece& victim = state.board[m.to.row][m.to.col];
    const Piece& attacker = state.board[m.from.row][m.from.col];

    if (!victim.IsEmpty()) {
        // MVV-LVA 启发式排序
        score += 10 * PIECE_VALUES[static_cast<int>(victim.type)] - PIECE_VALUES[static_cast<int>(attacker.type)];
    }
    if (m.isEnPassant) {
        score += 900;
    }
    if (m.promotion != PieceType::None) {
        score += PIECE_VALUES[static_cast<int>(m.promotion)];
    }
    if (m.isCastle) {
        score += 150;
    }
    return score;
}

int ChessEngine::Negamax(BoardState& state, int depth, int alpha, int beta) {
    if (depth == 0) {
        return EvaluateBoard(state);
    }

    std::vector<Move> moves = GenerateLegalMoves(state);
    if (moves.empty()) {
        if (IsInCheck(state, state.turn)) {
            return -(100000 + depth * 100);
        }
        return 0;
    }

    std::sort(moves.begin(), moves.end(), [&](const Move& a, const Move& b) {
        return MoveOrderScore(state, a) > MoveOrderScore(state, b);
    });

    int best = std::numeric_limits<int>::min() + 1;
    for (const Move& m : moves) {
        BoardState next = state;
        MakeMove(next, m);
        int score = -Negamax(next, depth - 1, -beta, -alpha);
        if (score > best) best = score;
        if (score > alpha) alpha = score;
        if (alpha >= beta) break;
    }
    return best;
}

Move ChessEngine::FindBestMove(const BoardState& state, int depth) {
    std::vector<Move> moves = GenerateLegalMoves(state);
    if (moves.empty()) return {};

    std::sort(moves.begin(), moves.end(), [&](const Move& a, const Move& b) {
        return MoveOrderScore(state, a) > MoveOrderScore(state, b);
    });

    Move bestMove = moves[0];
    int alpha = std::numeric_limits<int>::min() + 1;
    int beta = std::numeric_limits<int>::max();

    for (const Move& m : moves) {
        BoardState next = state;
        MakeMove(next, m);
        int score = -Negamax(next, depth - 1, -beta, -alpha);
        if (score > alpha) {
            alpha = score;
            bestMove = m;
        }
    }
    return bestMove;
}