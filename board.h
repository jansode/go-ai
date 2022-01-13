#ifndef BOARD_H
#define BOARD_H

#include <array>
#include <vector>
#include <unordered_set>
#include <string>

#include "parameters.h"

const int kBoardSize = 9;
const float kKomi = 6.5;

enum Color 
{
    EMPTY = 0,
    BLACK = 1,
    WHITE = 2,

    NUM_COLORS,
    COLOR_NONE 
};

enum Rules
{
    JAPANESE_RULES,
    CHINESE_RULES,

    NUM_RULES,
    RULES_NONE
};

// TODO: The name should be changed to point.
struct Coordinate
{
    int x;
    int y; 

    bool operator==(const Coordinate& other)
    {
        return other.x == x && other.y == y;
    }

    int As1D() const { return y*kBoardSize+x;};
    static Coordinate Get2dCoordinate(int value)
    {
        return {value % kBoardSize, value / kBoardSize};
    };
};

const std::vector<Coordinate> kStarpoints = {{2,2},{6,2},{2,6},{6,6}};
const std::vector<Coordinate> kSides = {{4,2},{2,4},{6,4},{4,6},{4,4},{4,5},{5,4},{5,5}};

struct Group
{
    int id;
    static int free_id;

    Group()
    {
        id = ++free_id;
    }

    std::vector<Coordinate> stones;
    std::vector<Coordinate> liberties;

    bool operator==(const Group& other)
    {
        return id = other.id;
    }
};

struct Move
{
    Coordinate point;
    std::vector<Group> captured_groups;

    int nr_captured_stones = 0;

    bool ko_active;
    Coordinate ko_point;

    bool white_passed;
    bool black_passed;
};

class Board
{

public:
    Board();

    // Returns true if the point is occupied.
    bool Occupied(const Coordinate& c);

    // Returns true if the point is occupied by the given side.
    bool OccupiedBy(const Coordinate& c, int side);

    // Makes a move on the given point on the board.
    // Returns true if the move was successfull.
    // Returns false if the move couldn't be made.
    bool MakeMove(const Coordinate& c);

    // Resets the board to the state
    // of the previous move.
    void UndoLastMove();

    // A move generation function that 
    // tries to gather a list of moves 
    // that should be further explored 
    // in a given position.
    //
    // I try to order the list from the 
    // move with the highest priority
    // to the one with the lowest priority.
    void GenerateMoves(std::vector<Coordinate> *moves, int side);

    // Adds the moves given in the new_moves list.
    void AddMovesInList(std::vector<Coordinate> *moves, std::unordered_set<int> *added_moves_set, std::vector<Coordinate> new_moves, int side, bool play_on_own_territory = false, bool play_on_opponents_influence = false, bool play_on_own_influence = false);

    // Add moves that are group liberties of 
    // groups for a given side with @liberties
    // number of liberties.
    void AddMovesOnGroupLiberties(std::vector<Coordinate> *moves, std::unordered_set<int>* added_moves_set, int side, int liberties);

    // Add moves that extend your influence.
    void AddMovesThatExtendYourInfluence(std::vector<Coordinate> *moves, std::unordered_set<int>* added_moves_set, int side, int moves_to_consider = 3);

    // Add all the moves that connect two groups.
    void AddMovesThatConnectTwoGroups(std::vector<Coordinate> *moves, std::unordered_set<int> *added_moves_set, int side);

    void AddMovesThatConnectGroupToEdge(std::vector<Coordinate> *moves, std::unordered_set<int> *added_move_set, int side);

    // Generates all the possible legal moves
    // in a given position.
    //
    // TODO: This should probably be changed so 
    // that the move ordering is also random.
    void GenerateRandomMoves(std::vector<Coordinate> *moves, std::unordered_set<int>* added_moves_set);

    // Returns an evaluation score for the
    // current board position. A positive score
    // is in black's favor and a negative one 
    // in white's favor.
    float Evaluate();

    // Calculates an evaluation score
    // on which side has the most influence on
    // a given point. A positive score is good
    // for black and a negative one is good for
    // white.
    int EvaluatePoint(const Coordinate& c);

    void CalculateInfluence();

    // Passes for the currently moving side.
    void Pass();

    // Returns true if both sides have passed.
    bool EndGame();

    // TODO: I think a flood fill algorithm
    // could be abstracted away for CheckForCaptures
    // and CalculateScore somehow and have these
    // call it with different arguments.

    // Returns false if the suicide rule or 
    // ko rule is violated and true if successful.
    bool CheckForCaptures(Move *current_move);

    // Calculates the territory occupied
    // for both sides.
    //
    // TODO: The name should be changed to CalculateTerritory.
    void CalculateScore(int rules);

    // Resets all the internal score data.
    void ResetScores();

    // Get stone at a point.
    // Returns WHITE, BLACK or EMPTY.
    int GetStone(int x, int y);

    // Get territory at a point.
    // Returns WHITE, BLACK or EMPTY.
    int GetTerritory(int x, int y);

    // Counts the liberties of a single point.
    int LibertiesOfPoint(const Coordinate& c);

    inline int GetSideToMove() const { return side_to_play;}
    inline int GetMovesPlayed() const { return moves_played;}

    int OppositeSide(int side);

    // Increment captured stones for 
    // a given side.
    void AddCaptures(int side, int amount);

    // Functions to write various board
    // information to stdout. 
    void Print();
    void PrintGroups();
    void PrintEvalBoard();
    void PrintEndScreen();
    void PrintTerritory();
    void PrintState();

private:
    void UpdateWhosTerritory(int value, int *whos_territory);

    // A stack to keep track of played moves
    // with information of captured groups.
    // This is needed for undoing moves.
    std::vector<Move> played_moves;

    bool white_passed = false;
    bool black_passed = false;

    int captures_white = 0;
    int captures_black = 0;

    int territory_white = 0;
    int territory_black = 0;

    // To keep track of ko-rule.
    // A single stone that was just captured 
    // cannot be immidiately recaptured.
    bool ko_active = false;
    Coordinate ko_point = {-1,-1};

    int moves_played = 0;

    int side_to_play = BLACK;

    std::vector<Group> white_groups;
    std::vector<Group> black_groups;

    // Internal arrays keeping track of occupied 
    // points, territory and evaluation.
    std::array<std::array<int,kBoardSize>,kBoardSize> board_array = {{EMPTY}};
    std::array<std::array<int,kBoardSize>,kBoardSize> territory_array = {{EMPTY}};
    std::array<std::array<int,kBoardSize>,kBoardSize> evaluation_array = {{EMPTY}};
};

#endif 
