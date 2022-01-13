#include "ai.h"
#include "parameters.h"
#include <iostream>
#include <random>
#include <chrono>
#include <vector>
#include <algorithm>

Ai::Ai(){}

float Ai::MiniMax(Board *board, int depth, float alpha, float beta)
{
    if(depth == 0)
        return board->Evaluate();

    int side_to_move = board->GetSideToMove();

    std::vector<Coordinate> moves;
    board->GenerateMoves(&moves, side_to_move);

    if(moves.size() == 0)
        return board->Evaluate();

    int moves_considered = 0;
    if(side_to_move == BLACK)
    {
        float value = -1000; 

        for(auto move : moves)
        {

            if(moves_considered++ == kMovesToConsider)
                    break;

            if(!board->MakeMove(move))
                continue;

            value = std::max(value, MiniMax(board,depth-1,alpha,beta));
            board->UndoLastMove();

            alpha = std::max(alpha, value);

            if(alpha >= beta)
            {
                //std::cout<<"Alpha cutoff\n";
                break;
            }
        }

        return value;
    }
    else
    {
        float value = 1000; 

        for(auto move : moves)
        {

            if(moves_considered++ == kMovesToConsider)
                    break;

            if(!board->MakeMove(move))
                continue;

            value = std::min(value, MiniMax(board,depth-1,alpha,beta));
            board->UndoLastMove();

            beta = std::min(beta, value);
            if(beta <= alpha)
            {
                //std::cout<<"Beta cutoff\n";
                break;
            }
        }

        return value;
    }
}

bool Ai::GetBestMove(Board *board, Coordinate *best_move)
{
    int side_to_move = board->GetSideToMove();

    std::vector<Coordinate> moves;
    board->GenerateMoves(&moves,side_to_move);

    Coordinate current_best = {-1,-1};

    if(moves.size() > 0)
    {
        current_best = moves[0];
    }
    else
        return false;

    // Don't search on the first few moves.
    // Just play some of the generated moves.
    if(board->GetMovesPlayed() < 4)
    {
        *best_move = moves[0];
        return true;
    }

    std::cout<<"AI thinking...\n";

    int current_eval = board->Evaluate();

    int moves_considered = 0;
    float value = -1000;

    for(auto move : moves)
    {
        if(moves_considered++ == kMovesToConsider)
            break;

        if(!board->MakeMove(move))
            continue;

#ifdef SEARCH_INFO 
        std::cout<<"Evaluation for move ("<<move.x<<","<<move.y<<"): ";
#endif

        float eval = MiniMax(board,kSearchDepth,-1000,1000);

#ifdef SEARCH_INFO
        std::cout<<eval<<"\n";
#endif

        if(value == -1000)
            value = eval;

        board->UndoLastMove();

        if(board->GetSideToMove() == BLACK && eval > value)
        {
            value = eval;
            current_best = move;
        }
        else if(board->GetSideToMove() == WHITE && eval < value)
        {
            value = eval;
            current_best = move;
        }

    }

    std::cout<<"Best move: ("<<current_best.x<<","<<current_best.y<<")\n"; 

    if(value == -1000 || value == 1000)
        return false;

    *best_move = current_best;

    if(current_best.x != -1)
    {
        bool ret = board->MakeMove(current_best);
        if(ret)
            board->UndoLastMove();

        return ret;
    }

    return false;
}

bool Ai::PlayMove(Board *board)
{
    int side_to_move = board->GetSideToMove();

    std::vector<Coordinate> moves;
    board->GenerateMoves(&moves,side_to_move);

    /*
    std::cout<<"Moves: \n";
    for(auto move : moves)
    {
        std::cout<<"("<<move.x<<","<<move.y<<")\n";
    }
    */

    Coordinate current_best = {-1,-1};

    if(moves.size() > 0)
        current_best = moves[0];
    else
        return false;

    // Don't search on the first few moves.
    // Just play some of the generated moves.
    if(board->GetMovesPlayed() < 4)
    {
        board->MakeMove(moves[0]);
        return true;
    }

    std::cout<<"AI thinking...\n";

    int current_eval = board->Evaluate();

    int moves_considered = 0;
    float value = -1000;

    for(auto move : moves)
    {
        if(moves_considered++ == kMovesToConsider)
            break;

        if(!board->MakeMove(move))
            continue;

#ifdef SEARCH_INFO 
        std::cout<<"Evaluation for move ("<<move.x<<","<<move.y<<"): ";
#endif

        float eval = MiniMax(board,kSearchDepth,-1000,1000);

#ifdef SEARCH_INFO
        std::cout<<eval<<"\n";
#endif

        if(value == -1000)
            value = eval;

        board->UndoLastMove();

        if(board->GetSideToMove() == BLACK && eval > value)
        {
            value = eval;
            current_best = move;
        }
        else if(board->GetSideToMove() == WHITE && eval < value)
        {
            value = eval;
            current_best = move;
        }

    }

    std::cout<<"Best move: ("<<current_best.x<<","<<current_best.y<<")\n"; 

    if(current_best.x != -1)
    {
        bool ret = board->MakeMove(current_best);
        return ret;
    }

    return false;

}
