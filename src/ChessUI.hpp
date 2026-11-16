#pragma once

#include "ChessEngine.hpp"
#include "ChessTypes.hpp"
#include <vector>

class ChessUI {
public:
    static constexpr int WINDOW_WIDTH = 920;
    static constexpr int WINDOW_HEIGHT = 720;
    static constexpr int CELL_SIZE = 76;
    static constexpr int BOARD_OFFSET_X = 48;
    static constexpr int BOARD_OFFSET_Y = 56;

    ChessUI();

    void Update();
    void Draw() const;

private:
    void Reset();
    void Undo();
    void ExecuteMove(const Move& move);
    bool GetSquareFromMouse(Pos& outPos) const;

    void DrawBoard() const;
    void DrawPiece(const Piece& piece, float cx, float cy, float size) const;
    void DrawSidePanel() const;
    void DrawPromotionModal() const;

    BoardState state_{};
    std::vector<Move> legalMoves_;
    std::vector<Snapshot> history_;

    Pos selectedSquare_{-1, -1};
    bool awaitingPromotion_ = false;
    Pos promoFrom_{-1, -1};
    Pos promoTo_{-1, -1};

    GameMode mode_ = GameMode::PvE;
    GameResult result_ = GameResult::Ongoing;
    float aiTimer_ = 0.0f;
};