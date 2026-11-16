#include "ChessUI.hpp"
#include "raylib.h"

#include <algorithm>
#include <string>

namespace {
constexpr Color BG_COLOR          = {34, 37, 43, 255};
constexpr Color PANEL_COLOR       = {44, 48, 56, 255};
constexpr Color LIGHT_SQ_COLOR    = {238, 216, 180, 255};
constexpr Color DARK_SQ_COLOR     = {181, 136, 99, 255};
constexpr Color SELECT_COLOR      = {246, 246, 105, 170};
constexpr Color LAST_MOVE_COLOR   = {186, 202, 68, 150};
constexpr Color CHECK_COLOR       = {220, 45, 45, 175};
constexpr Color HINT_DOT_COLOR    = {30, 30, 30, 90};
constexpr Color CAPTURE_RING_COLOR= {215, 45, 45, 190};
} // namespace

ChessUI::ChessUI() {
    Reset();
}

void ChessUI::Reset() {
    state_ = ChessEngine::CreateInitialState();
    legalMoves_ = ChessEngine::GenerateLegalMoves(state_);
    result_ = GameResult::Ongoing;
    history_.clear();
    selectedSquare_ = {-1, -1};
    awaitingPromotion_ = false;
    aiTimer_ = 0.0f;
}

void ChessUI::Undo() {
    if (history_.empty() || awaitingPromotion_) return;

    int steps = 1;
    if (mode_ == GameMode::PvE) {
        if ((state_.turn == PieceColor::White && history_.size() >= 2) ||
            (result_ == GameResult::BlackWin && history_.size() >= 2)) {
            steps = 2;
        }
    }

    for (int i = 0; i < steps && !history_.empty(); ++i) {
        state_ = history_.back().state;
        history_.pop_back();
    }

    legalMoves_ = ChessEngine::GenerateLegalMoves(state_);
    result_ = ChessEngine::EvaluateGameResult(state_, legalMoves_);
    selectedSquare_ = {-1, -1};
    aiTimer_ = 0.0f;
}

void ChessUI::ExecuteMove(const Move& move) {
    BoardState before = state_;
    ChessEngine::MakeMove(state_, move);
    std::string note = ChessEngine::MoveToNotation(before, move, state_);

    history_.push_back({before, move, true, note});
    legalMoves_ = ChessEngine::GenerateLegalMoves(state_);
    result_ = ChessEngine::EvaluateGameResult(state_, legalMoves_);
    selectedSquare_ = {-1, -1};
    awaitingPromotion_ = false;
    aiTimer_ = 0.0f;
}

bool ChessUI::GetSquareFromMouse(Pos& outPos) const {
    Vector2 mouse = GetMousePosition();
    int col = static_cast<int>((mouse.x - BOARD_OFFSET_X) / CELL_SIZE);
    int row = static_cast<int>((mouse.y - BOARD_OFFSET_Y) / CELL_SIZE);

    if (mouse.x < BOARD_OFFSET_X || mouse.y < BOARD_OFFSET_Y) return false;
    if (row < 0 || row >= 8 || col < 0 || col >= 8) return false;

    outPos = {row, col};
    return true;
}

void ChessUI::Update() {
    if (IsKeyPressed(KEY_R)) {
        Reset();
        return;
    }

    if (IsKeyPressed(KEY_M)) {
        mode_ = (mode_ == GameMode::PvE) ? GameMode::PvP : GameMode::PvE;
        Reset();
        return;
    }

    if (IsKeyPressed(KEY_Z) || IsKeyPressed(KEY_BACKSPACE)) {
        Undo();
        return;
    }

    if (result_ != GameResult::Ongoing) {
        return;
    }

    // 处理兵底线升变弹窗点击
    if (awaitingPromotion_) {
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            Vector2 m = GetMousePosition();
            constexpr float boxW = 300.0f;
            constexpr float boxH = 100.0f;
            float bx = BOARD_OFFSET_X + CELL_SIZE * 4.0f - boxW * 0.5f;
            float by = BOARD_OFFSET_Y + CELL_SIZE * 4.0f - boxH * 0.5f;

            constexpr PieceType choices[4] = {
                PieceType::Queen, PieceType::Rook, PieceType::Bishop, PieceType::Knight
            };

            for (int i = 0; i < 4; ++i) {
                Rectangle btn{bx + 12.0f + i * 70.0f, by + 28.0f, 62.0f, 62.0f};
                if (CheckCollisionPointRec(m, btn)) {
                    for (const Move& lm : legalMoves_) {
                        if (lm.from == promoFrom_ && lm.to == promoTo_ && lm.promotion == choices[i]) {
                            ExecuteMove(lm);
                            return;
                        }
                    }
                }
            }
        }
        return;
    }

    // PvE 模式下 AI（黑方）自动走棋
    if (mode_ == GameMode::PvE && state_.turn == PieceColor::Black) {
        aiTimer_ += GetFrameTime();
        if (aiTimer_ >= 0.15f) {
            Move best = ChessEngine::FindBestMove(state_, 3);
            if (best.from.IsValid()) {
                ExecuteMove(best);
            }
        }
        return;
    }

    // 玩家鼠标点击选子与落子
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        Pos clicked{};
        if (!GetSquareFromMouse(clicked)) {
            selectedSquare_ = {-1, -1};
            return;
        }

        if (selectedSquare_.IsValid()) {
            // 先检查是否点击了当前选中棋子的合法目标格
            std::vector<Move> matched;
            for (const Move& lm : legalMoves_) {
                if (lm.from == selectedSquare_ && lm.to == clicked) {
                    matched.push_back(lm);
                }
            }

            if (matched.size() == 1) {
                ExecuteMove(matched[0]);
                return;
            }
            if (matched.size() > 1) {
                // 兵升变有 4 个可选走法，弹出升变选择器
                awaitingPromotion_ = true;
                promoFrom_ = selectedSquare_;
                promoTo_ = clicked;
                return;
            }
        }

        // 选中己方棋子或取消选择
        if (state_.board[clicked.row][clicked.col].color == state_.turn) {
            selectedSquare_ = clicked;
        } else {
            selectedSquare_ = {-1, -1};
        }
    }
}

void ChessUI::DrawPiece(const Piece& piece, float cx, float cy, float size) const {
    if (piece.IsEmpty()) return;

    const bool isWhite = (piece.color == PieceColor::White);
    const Color fill = isWhite ? Color{248, 245, 236, 255} : Color{36, 38, 44, 255};
    const Color stroke = isWhite ? Color{45, 45, 45, 255} : Color{220, 215, 200, 255};
    const Color shadow = Fade(BLACK, 0.25f);
    const float s = size * 0.42f;

    auto drawBase = [&](float widthFactor) {
        Rectangle base{cx - s * widthFactor, cy + s * 0.62f, s * 2.0f * widthFactor, s * 0.26f};
        DrawRectangleRounded({base.x + 2, base.y + 2, base.width, base.height}, 0.4f, 4, shadow);
        DrawRectangleRounded(base, 0.4f, 4, fill);
        DrawRectangleRoundedLinesEx(base, 0.4f, 4, 1.8f, stroke);
    };

    switch (piece.type) {
        case PieceType::Pawn: {
            drawBase(0.65f);
            Vector2 v1{cx, cy - s * 0.35f};
            Vector2 v2{cx - s * 0.45f, cy + s * 0.62f};
            Vector2 v3{cx + s * 0.45f, cy + s * 0.62f};
            DrawTriangle(v1, v2, v3, fill);
            DrawTriangleLines(v1, v2, v3, stroke);
            DrawCircle(static_cast<int>(cx), static_cast<int>(cy - s * 0.42f), s * 0.32f, fill);
            DrawCircleLines(static_cast<int>(cx), static_cast<int>(cy - s * 0.42f), s * 0.32f, stroke);
            break;
        }

        case PieceType::Rook: {
            drawBase(0.80f);
            Rectangle body{cx - s * 0.48f, cy - s * 0.32f, s * 0.96f, s * 0.94f};
            DrawRectangleRec(body, fill);
            DrawRectangleLinesEx(body, 1.8f, stroke);

            Rectangle topBar{cx - s * 0.62f, cy - s * 0.56f, s * 1.24f, s * 0.26f};
            DrawRectangleRec(topBar, fill);
            DrawRectangleLinesEx(topBar, 1.8f, stroke);

            for (int i = -1; i <= 1; ++i) {
                Rectangle tooth{cx + i * s * 0.44f - s * 0.16f, cy - s * 0.80f, s * 0.32f, s * 0.26f};
                DrawRectangleRec(tooth, fill);
                DrawRectangleLinesEx(tooth, 1.8f, stroke);
            }
            break;
        }

        case PieceType::Knight: {
            drawBase(0.78f);
            // 马身与马头多边形轮廓
            Vector2 neck1{cx - s * 0.15f, cy - s * 0.78f};
            Vector2 neck2{cx - s * 0.48f, cy + s * 0.62f};
            Vector2 neck3{cx + s * 0.52f, cy + s * 0.62f};
            DrawTriangle(neck1, neck2, neck3, fill);
            DrawTriangleLines(neck1, neck2, neck3, stroke);

            Vector2 head1{cx - s * 0.12f, cy - s * 0.85f};
            Vector2 head2{cx - s * 0.68f, cy - s * 0.18f};
            Vector2 head3{cx + s * 0.42f, cy - s * 0.28f};
            DrawTriangle(head1, head2, head3, fill);
            DrawTriangleLines(head1, head2, head3, stroke);

            Vector2 ear1{cx - s * 0.05f, cy - s * 0.98f};
            Vector2 ear2{cx - s * 0.24f, cy - s * 0.68f};
            Vector2 ear3{cx + s * 0.16f, cy - s * 0.68f};
            DrawTriangle(ear1, ear2, ear3, fill);
            DrawTriangleLines(ear1, ear2, ear3, stroke);

            DrawCircle(static_cast<int>(cx - s * 0.28f), static_cast<int>(cy - s * 0.42f), s * 0.09f, stroke);
            break;
        }

        case PieceType::Bishop: {
            drawBase(0.75f);
            Vector2 b1{cx, cy - s * 0.72f};
            Vector2 b2{cx - s * 0.48f, cy + s * 0.62f};
            Vector2 b3{cx + s * 0.48f, cy + s * 0.62f};
            DrawTriangle(b1, b2, b3, fill);
            DrawTriangleLines(b1, b2, b3, stroke);

            DrawCircle(static_cast<int>(cx), static_cast<int>(cy - s * 0.25f), s * 0.36f, fill);
            DrawCircleLines(static_cast<int>(cx), static_cast<int>(cy - s * 0.25f), s * 0.36f, stroke);
            DrawCircle(static_cast<int>(cx), static_cast<int>(cy - s * 0.76f), s * 0.14f, fill);
            DrawCircleLines(static_cast<int>(cx), static_cast<int>(cy - s * 0.76f), s * 0.14f, stroke);

            DrawLineEx({cx - s * 0.16f, cy - s * 0.40f}, {cx + s * 0.16f, cy - s * 0.10f}, 2.2f, stroke);
            break;
        }

        case PieceType::Queen: {
            drawBase(0.84f);
            for (int i = -2; i <= 2; ++i) {
                float tipX = cx + i * s * 0.34f;
                float tipY = cy - s * (0.78f - std::abs(i) * 0.06f);
                Vector2 p1{tipX, tipY};
                Vector2 p2{cx - s * 0.42f, cy + s * 0.62f};
                Vector2 p3{cx + s * 0.42f, cy + s * 0.62f};
                DrawTriangle(p1, p2, p3, fill);
                DrawTriangleLines(p1, p2, p3, stroke);
                DrawCircle(static_cast<int>(tipX), static_cast<int>(tipY), s * 0.11f, fill);
                DrawCircleLines(static_cast<int>(tipX), static_cast<int>(tipY), s * 0.11f, stroke);
            }
            break;
        }

        case PieceType::King: {
            drawBase(0.84f);
            Rectangle crossV{cx - s * 0.10f, cy - s * 0.98f, s * 0.20f, s * 0.50f};
            Rectangle crossH{cx - s * 0.24f, cy - s * 0.84f, s * 0.48f, s * 0.18f};
            DrawRectangleRec(crossV, fill);
            DrawRectangleLinesEx(crossV, 1.6f, stroke);
            DrawRectangleRec(crossH, fill);
            DrawRectangleLinesEx(crossH, 1.6f, stroke);

            Vector2 k1{cx, cy - s * 0.56f};
            Vector2 k2{cx - s * 0.56f, cy + s * 0.62f};
            Vector2 k3{cx + s * 0.56f, cy + s * 0.62f};
            DrawTriangle(k1, k2, k3, fill);
            DrawTriangleLines(k1, k2, k3, stroke);

            DrawCircle(static_cast<int>(cx - s * 0.26f), static_cast<int>(cy - s * 0.18f), s * 0.32f, fill);
            DrawCircleLines(static_cast<int>(cx - s * 0.26f), static_cast<int>(cy - s * 0.18f), s * 0.32f, stroke);
            DrawCircle(static_cast<int>(cx + s * 0.26f), static_cast<int>(cy - s * 0.18f), s * 0.32f, fill);
            DrawCircleLines(static_cast<int>(cx + s * 0.26f), static_cast<int>(cy - s * 0.18f), s * 0.32f, stroke);
            break;
        }

        default:
            break;
    }
}

void ChessUI::DrawBoard() const {
    const int boardSizePx = CELL_SIZE * 8;
    DrawRectangleLinesEx(
        {static_cast<float>(BOARD_OFFSET_X - 3), static_cast<float>(BOARD_OFFSET_Y - 3),
         static_cast<float>(boardSizePx + 6), static_cast<float>(boardSizePx + 6)},
        3.0f, {110, 90, 70, 255});

    Pos kingInCheck{-1, -1};
    if (ChessEngine::IsInCheck(state_, state_.turn)) {
        kingInCheck = ChessEngine::FindKing(state_, state_.turn);
    }

    for (int r = 0; r < 8; ++r) {
        for (int c = 0; c < 8; ++c) {
            int x = BOARD_OFFSET_X + c * CELL_SIZE;
            int y = BOARD_OFFSET_Y + r * CELL_SIZE;
            Color sqColor = ((r + c) % 2 == 0) ? LIGHT_SQ_COLOR : DARK_SQ_COLOR;
            DrawRectangle(x, y, CELL_SIZE, CELL_SIZE, sqColor);

            Pos cur{r, c};
            if (!history_.empty()) {
                const Move& last = history_.back().lastMove;
                if (cur == last.from || cur == last.to) {
                    DrawRectangle(x, y, CELL_SIZE, CELL_SIZE, LAST_MOVE_COLOR);
                }
            }
            if (cur == selectedSquare_) {
                DrawRectangle(x, y, CELL_SIZE, CELL_SIZE, SELECT_COLOR);
            }
            if (cur == kingInCheck) {
                DrawRectangle(x, y, CELL_SIZE, CELL_SIZE, CHECK_COLOR);
            }

            DrawPiece(state_.board[r][c],
                      static_cast<float>(x + CELL_SIZE / 2),
                      static_cast<float>(y + CELL_SIZE / 2),
                      static_cast<float>(CELL_SIZE));
        }
    }

    // 绘制坐标标注 (a-h, 1-8)
    for (int i = 0; i < 8; ++i) {
        char rankStr[2] = {static_cast<char>('8' - i), '\0'};
        char fileStr[2] = {static_cast<char>('a' + i), '\0'};
        DrawText(rankStr, BOARD_OFFSET_X - 24, BOARD_OFFSET_Y + i * CELL_SIZE + CELL_SIZE / 2 - 8, 18, LIGHTGRAY);
        DrawText(fileStr, BOARD_OFFSET_X + i * CELL_SIZE + CELL_SIZE / 2 - 5, BOARD_OFFSET_Y + boardSizePx + 12, 18, LIGHTGRAY);
    }

    // 高亮选中棋子的所有合法目标点
    if (selectedSquare_.IsValid() && !awaitingPromotion_) {
        for (const Move& lm : legalMoves_) {
            if (lm.from != selectedSquare_) continue;
            int cx = BOARD_OFFSET_X + lm.to.col * CELL_SIZE + CELL_SIZE / 2;
            int cy = BOARD_OFFSET_Y + lm.to.row * CELL_SIZE + CELL_SIZE / 2;
            bool isCap = !state_.board[lm.to.row][lm.to.col].IsEmpty() || lm.isEnPassant;
            if (isCap) {
                DrawRing({static_cast<float>(cx), static_cast<float>(cy)},
                         CELL_SIZE * 0.36f, CELL_SIZE * 0.45f, 0.0f, 360.0f, 32, CAPTURE_RING_COLOR);
            } else {
                DrawCircle(cx, cy, CELL_SIZE * 0.16f, HINT_DOT_COLOR);
            }
        }
    }
}

void ChessUI::DrawSidePanel() const {
    const int px = BOARD_OFFSET_X + CELL_SIZE * 8 + 30;
    const int py = BOARD_OFFSET_Y;
    const int pw = WINDOW_WIDTH - px - 24;
    const int ph = CELL_SIZE * 8;

    DrawRectangleRounded({static_cast<float>(px), static_cast<float>(py),
                          static_cast<float>(pw), static_cast<float>(ph)},
                         0.03f, 6, PANEL_COLOR);

    DrawText("CHESS (MSVC)", px + 16, py + 16, 22, RAYWHITE);
    const char* modeLabel = (mode_ == GameMode::PvE) ? "Mode: PvE (vs AI)" : "Mode: PvP (2 Players)";
    DrawText(modeLabel, px + 16, py + 46, 16, SKYBLUE);

    const char* statusText = "";
    Color statusColor = GOLD;
    switch (result_) {
        case GameResult::Ongoing:
            statusText = (state_.turn == PieceColor::White) ? "Turn: White" : "Turn: Black";
            statusColor = (state_.turn == PieceColor::White) ? RAYWHITE : LIGHTGRAY;
            break;
        case GameResult::WhiteWin:
            statusText = "Checkmate! White Wins";
            statusColor = GREEN;
            break;
        case GameResult::BlackWin:
            statusText = "Checkmate! Black Wins";
            statusColor = ORANGE;
            break;
        case GameResult::Stalemate:
            statusText = "Draw (Stalemate)";
            statusColor = YELLOW;
            break;
        case GameResult::DrawInsufficient:
            statusText = "Draw (Insufficient)";
            statusColor = YELLOW;
            break;
        case GameResult::DrawFiftyMove:
            statusText = "Draw (50-Move Rule)";
            statusColor = YELLOW;
            break;
    }
    DrawText(statusText, px + 16, py + 78, 17, statusColor);

    DrawLine(px + 14, py + 110, px + pw - 14, py + 110, GRAY);
    DrawText("Move History:", px + 16, py + 120, 16, LIGHTGRAY);

    // 显示最近 14 个回合记录
    int totalFullMoves = (static_cast<int>(history_.size()) + 1) / 2;
    int startFullMove = std::max(0, totalFullMoves - 14);
    int yCursor = py + 146;

    for (int fm = startFullMove; fm < totalFullMoves; ++fm) {
        size_t wIdx = static_cast<size_t>(fm * 2);
        size_t bIdx = wIdx + 1;
        std::string line = std::to_string(fm + 1) + ". " + history_[wIdx].notation;
        if (bIdx < history_.size()) {
            line += "   " + history_[bIdx].notation;
        }
        DrawText(line.c_str(), px + 18, yCursor, 15, RAYWHITE);
        yCursor += 22;
    }

    DrawLine(px + 14, py + ph - 98, px + pw - 14, py + ph - 98, GRAY);
    DrawText("[R] Restart Game", px + 16, py + ph - 84, 15, LIGHTGRAY);
    DrawText("[Z] Undo Move", px + 16, py + ph - 60, 15, LIGHTGRAY);
    DrawText("[M] Switch PvE / PvP", px + 16, py + ph - 36, 15, LIGHTGRAY);
}

void ChessUI::DrawPromotionModal() const {
    if (!awaitingPromotion_) return;

    DrawRectangle(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, Fade(BLACK, 0.45f));

    constexpr float boxW = 300.0f;
    constexpr float boxH = 100.0f;
    float bx = BOARD_OFFSET_X + CELL_SIZE * 4.0f - boxW * 0.5f;
    float by = BOARD_OFFSET_Y + CELL_SIZE * 4.0f - boxH * 0.5f;

    DrawRectangleRounded({bx, by, boxW, boxH}, 0.1f, 8, {52, 56, 66, 255});
    DrawRectangleRoundedLinesEx({bx, by, boxW, boxH}, 0.1f, 8, 2.0f, GOLD);
    DrawText("Promote Pawn To:", static_cast<int>(bx + 16), static_cast<int>(by + 6), 16, GOLD);

    constexpr PieceType choices[4] = {
        PieceType::Queen, PieceType::Rook, PieceType::Bishop, PieceType::Knight
    };
    for (int i = 0; i < 4; ++i) {
        Rectangle btn{bx + 12.0f + i * 70.0f, by + 28.0f, 62.0f, 62.0f};
        DrawRectangleRec(btn, LIGHT_SQ_COLOR);
        DrawRectangleLinesEx(btn, 1.5f, DARK_SQ_COLOR);
        DrawPiece({choices[i], state_.turn}, btn.x + btn.width * 0.5f, btn.y + btn.height * 0.5f, 60.0f);
    }
}

void ChessUI::Draw() const {
    BeginDrawing();
    ClearBackground(BG_COLOR);

    DrawBoard();
    DrawSidePanel();
    DrawPromotionModal();

    EndDrawing();
}