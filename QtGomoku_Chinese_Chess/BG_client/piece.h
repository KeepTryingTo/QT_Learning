// piece.h
#ifndef PIECE_H
#define PIECE_H

#include <QObject>

enum class ChessPieceType {
    Empty = 0,
    General,        // 将/帅
    Advisor,        // 士/仕
    Elephant,       // 象/相
    Horse,          // 马/傌
    Rook,           // 车/俥
    Cannon,         // 炮/砲
    Pawn            // 兵/卒
};

enum class ChessPieceColor {
    Empty = 0,
    Red,
    Black
};

struct ChessPiece {
    ChessPieceType type;
    ChessPieceColor color;
    int x;
    int y;

    ChessPiece(ChessPieceType t = ChessPieceType::Empty,
               ChessPieceColor c = ChessPieceColor::Empty,
               int posX = -1, int posY = -1)
        : type(t), color(c), x(posX), y(posY) {}

    bool isEmpty() const { return type == ChessPieceType::Empty; }
    bool isRed() const { return color == ChessPieceColor::Red; }
    bool isBlack() const { return color == ChessPieceColor::Black; }
};

#endif // PIECE_H
