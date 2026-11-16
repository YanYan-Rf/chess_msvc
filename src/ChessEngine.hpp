#pragma once

#include "ChessTypes.hpp"
#include <string>
#include <vector>

class ChessEngine {
public:
    static BoardState CreateInitialState();
    static PieceColor Opposite(PieceColor c);

    static bool IsSquareAttacked(const BoardState& state, Pos sq, PieceColor byColor);
    static Pos FindKing(const BoardState& state, PieceColor color);
    static bool IsInCheck(const BoardState& state, PieceColor color);

    static std::vector<Move> GenerateLegalMoves(const BoardState& state);
    static void MakeMove(BoardState& state, const Move& move);

    static GameResult EvaluateGameResult(const BoardState& state, const std::vector<Move>& legalMoves);
    static std::string MoveToNotation(const BoardState& beforeState, const Move& move, const BoardState& afterState);

    // AI ËÑË÷£¨Negamax + Alpha-Beta£©
    static Move FindBestMove(const BoardState& state, int depth = 3);

private:
    static std::vector<Move> GeneratePseudoLegalMoves(const BoardState& state);
    static int EvaluateBoard(const BoardState& state);
    static int Negamax(BoardState& state, int depth, int alpha, int beta);
    static int MoveOrderScore(const BoardState& state, const Move& m);
};