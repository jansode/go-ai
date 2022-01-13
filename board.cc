#include "board.h"
#include "parameters.h"
#include "util.h"
#include <iostream>
#include <algorithm>

int Group::free_id = 0;

Board::Board()
{}

int Board::LibertiesOfPoint(const Coordinate& c)
{
    int liberties = 0;
    if(c.x > 0)
    {
        if(!Occupied({c.x-1,c.y})) ++liberties;
    }
    if(c.x < kBoardSize - 1)
    {
        if(!Occupied({c.x+1,c.y})) ++liberties;
    }
    if(c.y < kBoardSize - 1)
    {
        if(!Occupied({c.x,c.y+1})) ++liberties;
    }
    if(c.y > 0)
    {
        if(!Occupied({c.x,c.y-1})) ++liberties;
    }

    return liberties;
}

void Board::CalculateInfluence()
{
    for(int x = 0; x < kBoardSize; ++x)
    {
        for(int y = 0; y < kBoardSize; ++y)
        {
            if(!Occupied({x,y}))
                evaluation_array[x][y] = EvaluatePoint({x,y});
        }
    }
}

float Board::Evaluate()
{
    CalculateInfluence();
    CalculateScore(JAPANESE_RULES);

    float score = 0;

    for(int x = 0; x < kBoardSize; ++x)
    {
        for(int y = 0; y < kBoardSize; ++y)
        {
            score += evaluation_array[x][y];
        }
    }

    if(score > 0)
        score = kInfluenceWeight;
    else if(score < 0)
        score = -kInfluenceWeight;


    int liberties_white = 0;
    for(auto group : white_groups)
        liberties_white += group.liberties.size();

    int liberties_black = 0;
    for(auto group : black_groups)
        liberties_black += group.liberties.size();

    if(liberties_white > liberties_black)
        score -= kLibertiesWeight;
    else if(liberties_white < liberties_black)
        score += kLibertiesWeight;

    score -= captures_white*kCapturesWeight;
    score += captures_black*kCapturesWeight;

    score -= territory_white*kTerritoryWeight;
    score += territory_black*kTerritoryWeight;

    score -= kKomi;

    return score;
}

bool Board::Occupied(const Coordinate& c)
{
    return board_array[c.x][c.y] != EMPTY;
}

bool Board::OccupiedBy(const Coordinate& c, int side)
{
    return board_array[c.x][c.y] == side;
}

int Board::GetStone(int x, int y)
{
    return board_array[x][y];
}

int Board::GetTerritory(int x, int y)
{
    return territory_array[x][y];
}

int Board::EvaluatePoint(const Coordinate& c)
{
    if(!Occupied(c))
    {
        int weight = kPointEvaluation;
        int majority = 0;
        for(int i = -2; i < 3; ++i)
        {
            for(int j = -2; j < 3; ++j)
            {
                if(c.x+i >= 0 && c.x+i < kBoardSize && c.y+j >= 0 && c.y+j < kBoardSize)
                {
                    // Edge points are weighed more heavily.
                    if(c.x == 0 || c.x == 8 || c.y == 0 || c.y == 8)
                    {
                        weight = kEdgePointEvaluation;
                    }
                    else if(c.x == 1 || c.x == 7 || c.y == 1 || c.y == 7)
                    {
                        weight = kEdgePointEvaluation;
                    }

                    if(board_array[c.x+i][c.y+j] == BLACK)
                        majority += weight;
                    else if(board_array[c.x+i][c.y+j] == WHITE)
                        majority -= weight;
                }
            }
        }

        return majority;
    }
    return COLOR_NONE;
}

bool Board::MakeMove(const Coordinate& c)
{
    if(c.x < 0 || c.x >= kBoardSize || c.y < 0 || c.y >= kBoardSize)
    {
        std::cout<<"Coordinates need to be between 0 and 9.\n";
        return false;
    }

    if(Occupied(c)) 
    {
        std::cout<<"The selected coordinate is already occupied!\n"; 
        return false;
    }


    Move current_move;
    current_move.point = c;
    current_move.ko_active = ko_active;
    current_move.ko_point = ko_point;

    if(ko_active)
    {
        if(ko_point.x == c.x && ko_point.y == c.y)
            return false;
    }

    ko_active = false;
    ko_point = {-1,-1};

    board_array[c.x][c.y] = side_to_play;

    if(!CheckForCaptures(&current_move))
    {
        // Suicide rule has been violated.
        // Undo the move.
        board_array[c.x][c.y] = EMPTY;
        //std::cout<<"Suicide rule or ko rule was violated.\n";
        return false;
    }



    if(side_to_play == BLACK)
    {
        black_passed = false;
        side_to_play = WHITE;
    }
    else
    {
        white_passed = false;
        side_to_play = BLACK;
    }

    played_moves.push_back(current_move);
    ++moves_played;


    return true;
}

void Board::UndoLastMove()
{
    if(moves_played == 0) return;

    Move& played_move = played_moves.back();

    board_array[played_move.point.x][played_move.point.y] = EMPTY;

    if(played_move.captured_groups.size() > 0) 
    {
        // We need to restore the stones that were previously
        // captured.
        for(auto group : played_move.captured_groups)
        {
            for(auto stone : group.stones)
            {
                board_array[stone.x][stone.y] = side_to_play;
            }
        }
    }

    ko_active = played_move.ko_active;
    ko_point = played_move.ko_point;

    if(side_to_play == WHITE)
    {
        side_to_play = BLACK;
        captures_black -= played_move.nr_captured_stones;
    }
    else
    {
        side_to_play = WHITE;
        captures_white -= played_move.nr_captured_stones;
    }

    played_moves.pop_back();
    --moves_played;
}

int Board::OppositeSide(int side)
{
    if(side == BLACK) return WHITE;

    return BLACK;
}

void Board::AddCaptures(int side, int amount)
{
    if(side == WHITE)
        captures_white += amount;
    else
        captures_black += amount;
}

void Board::AddMovesOnGroupLiberties(std::vector<Coordinate> *moves, std::unordered_set<int> *added_moves_set, int side, int liberties)
{
    std::vector<Group>& all_groups = (side == WHITE)?white_groups:black_groups;

    for(auto group : all_groups)
    {
        if(group.liberties.size() == liberties)
        {
            for(auto liberty : group.liberties)
            {
                if(!Occupied(liberty) && !Util::set_contains<int>(added_moves_set,liberty.As1D()))
                {
                    moves->push_back(liberty);
                    added_moves_set->insert(liberty.As1D());
                }
            }
        }
    }
}

void Board::AddMovesInList(std::vector<Coordinate> *moves, std::unordered_set<int>* added_moves_set,std::vector<Coordinate> new_moves, int side, bool play_on_own_territory, bool play_on_opponents_influence, bool play_on_own_influence)
{
    for(auto c : new_moves)
    {
        if(!play_on_own_territory)
        {
            // Don't play moves if the territory is already 
            // occupied by the given side.
            if(territory_array[c.x][c.y] == side)
                continue;
        }

        if(!play_on_opponents_influence)
        {
            // Don't play the move if it is under opposite influence.
            if(side == WHITE)
            {
                if(evaluation_array[c.x][c.y] > 0)
                    continue;
            }
            else
            {
                if(evaluation_array[c.x][c.y] < 0)
                    continue;
            }
        }

        if(!play_on_own_influence)
        {
            // Don't play the move if it is under opposite influence.
            if(side == WHITE)
            {
                if(evaluation_array[c.x][c.y] < 0)
                    continue;
            }
            else
            {
                if(evaluation_array[c.x][c.y] > 0)
                    continue;
            }
        }

        if(!Occupied(c) && !Util::set_contains<int>(added_moves_set,c.As1D()))
        {
            moves->push_back(c);
            added_moves_set->insert(c.As1D());
        }
    }
}

void Board::AddMovesThatConnectTwoGroups(std::vector<Coordinate> *moves, std::unordered_set<int> *added_moves_set, int side)
{
    std::vector<Group>& groups = (side == WHITE)?white_groups:black_groups;

    // Connect groups.
    for(auto g1 : groups)
    {
        for(auto g2 : groups)
        {
            if(g1 == g2) continue;

            for(auto l1 : g1.liberties)
            {
                // Don't play on your own territory unless you have to.
                if(territory_array[l1.x][l1.y] == side)
                    continue;

                for(auto l2 : g2.liberties)
                {
                    if(l1 == l2 && !Util::set_contains<int>(added_moves_set,l1.As1D()))
                    {
                        moves->push_back(l1);
                        added_moves_set->insert(l1.As1D());
                    }
                }
            }

        }
    }
}

void Board::AddMovesThatExtendYourInfluence(std::vector<Coordinate> *moves, std::unordered_set<int>* added_moves_set, int side, int moves_to_consider)
{
    // What this basically does is that it 
    // finds the lowest influence points
    // of the opposite side in the evaluation array
    // and adds these as potential moves.
    
    std::vector<std::pair<int, Coordinate>> sorted_moves;
    for(int x = 0; x < kBoardSize; ++x)
    {
        for(int y = 0; y < kBoardSize; ++y)
        {
            if(!Occupied({x,y}))
            {
                if((x == 0 && y == 0) || (x == 0 && y == 8) || (x == 8 && y == 8) || (x == 8 && y ==0))
                {
                    // Avoid playing the corner point for no good reason.
                    if(LibertiesOfPoint({x,y}) == 2) 
                        continue;
                }
                if(x == 0 || y == 0 || x == 8 || y == 8)
                {
                    // Avoid playing the edge points for no good reason.
                    if(LibertiesOfPoint({x,y}) == 3) 
                        continue;
                }

                // Don't play on your own territory unless you have to.
                if(territory_array[x][y] == side)
                    continue;

                // Don't play on your opponents territory unless you have to.
                if(territory_array[x][y] == OppositeSide(side))
                    continue;

                /*
                // Don't play on points that have opposing influence..
                if(evaluation_array[x][y] == OppositeSide(side))
                    continue;
                    */

                Coordinate c = {x,y};
                int value = evaluation_array[x][y];
                if(side == WHITE && value >= 0)
                {
                    sorted_moves.push_back(std::make_pair(value,c));
                }
                else if(side == BLACK && value <= 0)
                {
                    sorted_moves.push_back(std::make_pair(value,c));
                }
            }
        }
    }

    if(side == WHITE)
    {
        sort(sorted_moves.begin(),sorted_moves.end(),
        [](const std::pair<int,Coordinate>& c1, const std::pair<int,Coordinate>& c2)
        {
            return c1.first < c2.first;
        });
    }
    else
    {
        sort(sorted_moves.begin(),sorted_moves.end(),
        [](const std::pair<int,Coordinate>& c1, const std::pair<int,Coordinate>& c2)
        {
            return c1.first > c2.first;
        });
    }

    for(int i=0; i < sorted_moves.size(); ++i)
    {
        if(i == moves_to_consider) break;

        if(!Util::set_contains<int>(added_moves_set,sorted_moves[i].second.As1D()))
        {
            moves->push_back(sorted_moves[i].second);
            added_moves_set->insert(sorted_moves[i].second.As1D());
        }
    } 
}

void Board::AddMovesThatConnectGroupToEdge(std::vector<Coordinate> *moves, std::unordered_set<int> *added_moves_set, int side)
{
    for(int x = 1; x < kBoardSize - 1; ++x)
    {
        Coordinate c1 = {x,0};
        Coordinate c2 = {x,8};

        if(!Occupied({x,0}) && !Util::set_contains<int>(added_moves_set, c1.As1D())) 
        {
            if(OccupiedBy({x,1},side))
            {
                moves->push_back(c1);
                added_moves_set->insert(c1.As1D());
            }
            if(OccupiedBy({x+1,1},side))
            {
                moves->push_back(c1);
                added_moves_set->insert(c1.As1D());
            }
            if(OccupiedBy({x-1,1},side))
            {
                moves->push_back(c1);
                added_moves_set->insert(c1.As1D());
            }
        }

        if(!Occupied(c2) && !Util::set_contains<int>(added_moves_set, c2.As1D())) 
        {
            if(OccupiedBy({x,7},side))
            {
                moves->push_back(c2);
                added_moves_set->insert(c2.As1D());
            }
            if(OccupiedBy({x+1,7},side))
            {
                moves->push_back(c2);
                added_moves_set->insert(c2.As1D());
            }
            if(OccupiedBy({x-1,7},side))
            {
                moves->push_back(c2);
                added_moves_set->insert(c2.As1D());
            }
        }
    }

    for(int y = 1; y < kBoardSize - 1; ++y)
    {
        Coordinate c1 = {0,y};
        Coordinate c2 = {8,y};

        if(!Occupied(c1) && !Util::set_contains<int>(added_moves_set, c1.As1D())) 
        {
            if(OccupiedBy({1,y},side))
            {
                moves->push_back(c1);
                added_moves_set->insert(c1.As1D());
            }
            if(OccupiedBy({1,y+1},side))
            {
                moves->push_back(c1);
                added_moves_set->insert(c1.As1D());
            }
            if(OccupiedBy({1,y-11},side))
            {
                moves->push_back(c1);
                added_moves_set->insert(c1.As1D());
            }
        }

        if(!Occupied(c2) && !Util::set_contains<int>(added_moves_set, c2.As1D())) 
        {
            if(OccupiedBy({7,y},side))
            {
                moves->push_back(c2);
                added_moves_set->insert(c2.As1D());
            }
            if(OccupiedBy({7,y+1},side))
            {
                moves->push_back(c2);
                added_moves_set->insert(c2.As1D());
            }
            if(OccupiedBy({7,y-1},side))
            {
                moves->push_back(c2);
                added_moves_set->insert(c2.As1D());
            }
        }
    }
}

void Board::GenerateMoves(std::vector<Coordinate> *moves, int side)
{
    int opponent_side = OppositeSide(side);

    CalculateInfluence();

    // Keep track of already_added_moves.
    std::unordered_set<int> added_moves_set;

    // Look for own groups with 1 liberty 
    // See if these should be saved.
    AddMovesOnGroupLiberties(moves, &added_moves_set, side, 1);

    // Look for opponents groups with 1 liberty 
    // See if these should be captured.
    AddMovesOnGroupLiberties(moves, &added_moves_set, opponent_side, 1);

    // Look for own groups with 2 liberties 
    // See if these should be defended.
    //AddMovesOnGroupLiberties(moves, &added_moves_set, side, 2);

    // Try to take star points if available.
    AddMovesInList(moves, &added_moves_set, kStarpoints, side, false, (moves_played <= 1),true);

    // Try to take sides if available.
    AddMovesInList(moves, &added_moves_set, kSides, side, false, (moves_played <= 1),true);

    AddMovesThatConnectGroupToEdge(moves, &added_moves_set, side);

    // Try to extend your influence on the board.
    AddMovesThatExtendYourInfluence(moves, &added_moves_set, side, 7);

    /*
    // Look for opponents groups with 2 liberties 
    // See if these can be attacked.
    AddMovesOnGroupLiberties(moves, &added_moves_set, opponent_side, 2);
    */


    // Try to stay connected.
    AddMovesThatConnectTwoGroups(moves, &added_moves_set, side);

    // If nothing else, generate all the remaining moves. 
    //GenerateRandomMoves(moves, &added_moves_set);
}

void Board::GenerateRandomMoves(std::vector<Coordinate> *moves, std::unordered_set<int> *added_moves_set)
{
    for(int x = 0; x < kBoardSize; ++x)
    {
        for(int y = 0; y < kBoardSize; ++y)
        {
            Coordinate c = {x,y};
            if(!Occupied(c) && !Util::set_contains<int>(added_moves_set,c.As1D())) 
            {
                moves->push_back(c);
                added_moves_set->insert(c.As1D());
            }
        }
    }
}

bool Board::CheckForCaptures(Move *current_move)
{
    white_groups.clear();
    black_groups.clear();

    int side_to_check = OppositeSide(side_to_play);

    // We need to go through all the 
    // possible captures first before 
    // deciding on if the move is a suicide.
    // An otherwise suicidal move that 
    // captures opposing stones is allowed.
    bool possible_suicide = false;

    // The way this works is that we go 
    // through each square on the board
    // looking for a stone and keeping track
    // of already visited points. When we find a
    // stone we do a flood fill starting from 
    // that point with the empty points and 
    // opposing stones as boundaries.
    //
    // The flood fill stores the stones
    // it encounters and the amount 
    // of liberties that the group has.
    // If the number of liberties is 0
    // after the filling, we know that 
    // these stones should be removed 
    // from the board.
    
    std::unordered_set<int> checked;
    for(int x=0; x<kBoardSize; ++x)
    {
        for(int y=0; y<kBoardSize; ++y)
        {
            Coordinate c = {x,y};
            auto found = checked.find(c.As1D());
            if(found == checked.end()) 
            {
                checked.insert(c.As1D());

                if(board_array[x][y] == EMPTY)
                    continue;

                side_to_check = board_array[x][y];

                std::vector<Coordinate> group_stack;
                std::vector<Coordinate> potential_captures;
                std::vector<Coordinate> liberties;

                group_stack.push_back(c);
                potential_captures.push_back(c);

                auto check_neighbor = [&](const Coordinate& c)
                {
                    int value = board_array[c.x][c.y];
                    if(value == EMPTY)
                    {
                        if(std::find(liberties.begin(),liberties.end(),c) == liberties.end())
                            liberties.push_back({c.x,c.y});

                        return;
                    }

                    auto found = checked.find(c.As1D());
                    if(found == checked.end())
                    {
                        if(value == side_to_check)
                        {
                            group_stack.push_back({c.x,c.y});
                            potential_captures.push_back({c.x,c.y});
                            checked.insert(c.As1D());
                        }
                    }
                };

                do
                {
                    c = group_stack.back();
                    group_stack.pop_back();

                    // Check all the neighbors of 
                    // the current coordinate.
                    // If it contains a stone of 
                    // the checked color, then
                    // add it to both the group_stack and 
                    // potential_captures vectors.  
                    if(c.x > 0)                     
                        check_neighbor({c.x-1,c.y});
                    if(c.x < kBoardSize - 1)                     
                        check_neighbor({c.x+1,c.y});
                    if(c.y > 0)                     
                        check_neighbor({c.x,c.y-1});
                    if(c.y < kBoardSize - 1)                     
                        check_neighbor({c.x,c.y+1});
                }
                while(!group_stack.empty());

                if(liberties.size() == 0)
                {
                    // If this happens, it means that a
                    // suicidal move has been played.
                    // We still need to check all the
                    // possible captures in case the 
                    // move is legal after all. 
                    if(board_array[x][y] == side_to_play)
                    {
                        possible_suicide = true;
                        continue;
                    }

                    // The group is dead. Update the board and score accordingly.
                    int number_of_captured = potential_captures.size();
                    AddCaptures(side_to_play,number_of_captured);

                    if(number_of_captured == 1)
                    {
                        ko_active = true;
                        ko_point = potential_captures[0];
                    }

                    Group captured_group;
                    captured_group.stones = potential_captures;
                    
                    current_move->captured_groups.push_back(captured_group);
                    current_move->nr_captured_stones += potential_captures.size();

                    for(auto c : potential_captures) 
                        board_array[c.x][c.y] = EMPTY;

                    possible_suicide = false;

                }
                else
                {
                    Group new_group; 
                    new_group.stones = potential_captures;
                    new_group.liberties = liberties;

                    // Update group vectors for alive groups.
                    if(board_array[x][y] == WHITE)
                        white_groups.push_back(new_group);
                    else
                        black_groups.push_back(new_group);

                }
            }
        }
    }

    if(possible_suicide) return false;

    return true;

}

void Board::UpdateWhosTerritory(int value, int *whos_territory)
{
    switch(*whos_territory)
    {
        case EMPTY: *whos_territory = value; break;
        case BLACK:
        {
            if(value == WHITE)
                *whos_territory = COLOR_NONE;
            break;
        }
        case WHITE:
        {
            if(value == BLACK)
                *whos_territory = COLOR_NONE;
            break;
        }
    }
}

void Board::ResetScores()
{
    territory_white = 0;
    territory_black = 0;
    for(int y = 0; y < kBoardSize; ++y)
    {
        for(int x = 0; x < kBoardSize; ++x)
        {
            territory_array[x][y] = COLOR_NONE;
        }
    }

}

void Board::CalculateScore(int rules)
{
    ResetScores();

    // This is basically the same algorithm
    // as CheckForCaptures, but now we're 
    // doing a flood fill on the empty squares
    // with the stones as boundaries.
    // If we only encounter one type of 
    // stone during the filling, we know that
    // the territory belongs to that player.
    
    std::unordered_set<int> checked;
    for(int y = 0; y < kBoardSize; ++y)
    {
        for(int x = 0; x < kBoardSize; ++x)
        {
            int value = board_array[x][y];

            if(value != EMPTY) continue;

            Coordinate c = {x,y};
            auto found = checked.find(c.As1D());
            if(found == checked.end())
            {
                checked.insert(c.As1D());

                std::vector<Coordinate> territory_stack;
                std::vector<Coordinate> potential_territory;

                // Determines who's territory the coordinates
                // are counted as. COLOR_NONE means that the 
                // territory hasn't been surrounded by either 
                // player.
                int whos_territory = EMPTY;

                territory_stack.push_back(c);
                potential_territory.push_back(c);

                auto check_neighbor = [&](const Coordinate& c)
                {
                    int value = board_array[c.x][c.y];
                    if(value == EMPTY)
                    {
                        found = checked.find(c.As1D());
                        if(found == checked.end())
                        { 
                            territory_stack.push_back(c);
                            potential_territory.push_back(c);
                            checked.insert(c.As1D());
                        }
                                                    
                    }
                    else
                        UpdateWhosTerritory(value,&whos_territory);
                };

                do
                {
                    c = territory_stack.back();
                    territory_stack.pop_back();

                    if(c.x > 0)                     
                        check_neighbor({c.x-1,c.y});
                    if(c.x < kBoardSize - 1)                     
                        check_neighbor({c.x+1,c.y});
                    if(c.y > 0)                     
                        check_neighbor({c.x,c.y-1});
                    if(c.y < kBoardSize - 1)                     
                        check_neighbor({c.x,c.y+1});
                }
                while(!territory_stack.empty());

                if(whos_territory != COLOR_NONE && whos_territory != EMPTY)
                {
                    if(whos_territory == WHITE)
                        territory_white += potential_territory.size();
                    else
                        territory_black += potential_territory.size();

                    // The territory has a definite owner.
                    // Update the territory board accordingly.
                    for(auto c : potential_territory)
                        territory_array[c.x][c.y] = whos_territory;
                }
            }
        }
    }

}

void Board::Pass()
{
    if(side_to_play == WHITE)
    {
        side_to_play = BLACK;
        white_passed = true;
    }
    else
    {
        side_to_play = WHITE;
        black_passed = true;
    }

}

void Board::Print()
{
    for(int i=0; i<kBoardSize; ++i)
    {
        for(int j=0; j<kBoardSize; ++j)
        {
            switch(board_array[j][i]) 
            {
                case EMPTY:
                    std::cout<<".";
                    break;
                case BLACK:
                    std::cout<<"b";
                    break;
                case WHITE:
                    std::cout<<"w";
                    break;
            }
        }
        std::cout<<"\n";
    }

    std::cout<<"Captures white: "<<captures_white<<"\n";
    std::cout<<"Captures black: "<<captures_black<<"\n";
}

void Board::PrintTerritory()
{
    for(int i=0; i<kBoardSize; ++i)
    {
        for(int j=0; j<kBoardSize; ++j)
        {
            switch(territory_array[j][i]) 
            {
                case EMPTY:
                case COLOR_NONE:
                    std::cout<<".";
                    break;
                case BLACK:
                    std::cout<<"b";
                    break;
                case WHITE:
                    std::cout<<"w";
                    break;
            }
        }
        std::cout<<"\n";
    }
    std::cout<<"Territory white: "<<territory_white<<"\n";
    std::cout<<"Territory black: "<<territory_black<<"\n";
}

void Board::PrintEvalBoard()
{
    for(int i=0; i<kBoardSize; ++i)
    {
        for(int j=0; j<kBoardSize; ++j)
        {
            int eval = EvaluatePoint({j,i});
            if(Occupied({j,i}) || eval == 0) 
                std::cout<<".";
            else if(eval > 0)
                std::cout<<"b";
            else
                std::cout<<"w";
        }
        std::cout<<"\n";
    }
}

void Board::PrintState()
{
    std::cout<<"Move: : "<<moves_played+1<<"\n";
    std::cout<<"Territory white: "<<territory_white<<"\n";
    std::cout<<"Territory black: "<<territory_black<<"\n\n";
    std::cout<<"Captures white: "<<captures_white<<"\n";
    std::cout<<"Captures black: "<<captures_black<<"\n\n";
}

void Board::PrintGroups()
{
    std::cout<<"White has "<<white_groups.size()<<" groups:\n";
    for(int i = 0; i < white_groups.size(); ++i) 
    {
        std::cout<<"Group "<<i<<" has "<<white_groups[i].liberties.size()<<" liberties.\n";
    }

    std::cout<<"\n";

    std::cout<<"Black has "<<black_groups.size()<<" groups:\n";
    for(int i = 0; i < black_groups.size(); ++i) 
    {
        std::cout<<"Group "<<i<<" has "<<black_groups[i].liberties.size()<<" liberties.\n";
    }
}

void Board::PrintEndScreen()
{
    int winner = COLOR_NONE;

    int white_score = captures_white + territory_white + 6.5;
    int black_score = captures_black + territory_black;

    if(white_score > black_score)
    {
        std::cout<<"White won by " <<white_score - black_score<<" points.\n";
    }
    else
    {
        std::cout<<"Black won by " <<black_score - white_score<<" points.\n";
    }
}

bool Board::EndGame()
{
    return white_passed && black_passed;
}
