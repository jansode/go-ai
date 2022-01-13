#ifndef AI_H
#define AI_H

#include "board.h"

const float kNoPreviousEvaluation = 1000;

class Ai
{
public:
    Ai();
    bool PlayMove(Board *board);

    bool GetBestMove(Board *board, Coordinate *best_move);

    float MiniMax(Board *board, int depth, float alpha, float beta);

private:
    float previous_evaluation = kNoPreviousEvaluation;
};

#endif
