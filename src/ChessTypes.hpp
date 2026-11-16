#pragma once

#include <array>
#include <string>

enum class PieceType {
    None = 0,
    Pawn,
    Rook,
    Knight,
    Bishop,
    Queen,
    King
};

enum class PieceColor {
    None = 0,
    White,
    Black
};

enum class GameMode {
    PvE,
    PvP
};

enum class GameResult {
    Ongoing,
    WhiteWin,
    BlackWin,
    Stalemate,
    DrawInsufficient,
    DrawFiftyMove
};

struct Pos {
    int row = -1;
    int col = -1;

    bool IsValid() const {
        return row >= 0 && row < 8 && col >= 0 && col < 8;
    }

    bool operator==(const Pos& other) const {
        return row == other.row && col == other.col;
    }

    bool operator!=(const Pos& other) const {
        return !(*this == other);
    }
};

struct Piece {
    PieceType type = PieceType::None;
    PieceColor color = PieceColor::None;

    bool IsEmpty() const {
        return type == PieceType::None || color == PieceColor::None;
    }
};

struct Move {
    Pos from{};
    Pos to{};
    PieceType promotion = PieceType::None;
    bool isEnPassant = false;
    bool isCastle = false;

    bool operator==(const Move& other) const {
        return from == other.from && to == other.to &&
               promotion == other.promotion &&
               isEnPassant == other.isEnPassant &&
               isCastle == other.isCastle;
    }
};

struct BoardState {
    std::array<std::array<Piece, 8>, 8> board{};
    PieceColor turn = PieceColor::White;

    bool whiteCastleK = true;
    bool whiteCastleQ = true;
    bool blackCastleK = true;
    bool blackCastleQ = true;

    Pos enPassantTarget{-1, -1};
    int halfMoveClock = 0;
};

struct Snapshot {
    BoardState state{};
    Move lastMove{};
    bool hasLastMove = false;
    std::string notation;
};