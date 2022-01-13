#include "go_game.h"
#include <SDL2/SDL.h>
#include <iostream>

GoGame::GoGame()
{}

GoGame::~GoGame()
{
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

bool GoGame::Init(int window_width, int window_height)
{
    if(SDL_Init(SDL_INIT_VIDEO) != 0)
        return false;

    if(SDL_CreateWindowAndRenderer(window_width, window_height, 0, &window, &renderer) < 0)
        return false;

    SDL_SetWindowTitle(window,"Go");

    return true;
}

void GoGame::DrawCircle(int radius, int x, int y, const SDL_Color& color)
{
    SDL_SetRenderDrawColor(renderer,color.r,color.g,color.b,color.a);
    for(int w = 0; w < radius * 2; ++w)
    {
        for(int h = 0; h < radius * 2; ++h)
        {
            int dx = radius - w;
            int dy = radius - h;
            if((dx*dx + dy*dy) <= (radius * radius))
            {
                SDL_RenderDrawPoint(renderer,x + dx, y + dy);
            }
        }
    }
}

void GoGame::DrawStone(int x, int y, int color)
{
    SDL_Color stone_color; 

    if(color == WHITE) stone_color = {255,255,255,255};
    else stone_color = {0,0,0,255};

    DrawCircle(20,padding_left+cell_size*x,padding_up+cell_size*y,stone_color);
}

void GoGame::DrawTerritoryMarker(int x, int y, int color)
{
    SDL_Color territory_color; 

    if(color == WHITE) territory_color = {255,255,255,255};
    else territory_color = {0,0,0,255};


    SDL_SetRenderDrawColor(renderer,territory_color.r,territory_color.g,territory_color.b,territory_color.a);
    SDL_Rect rect = {padding_left+cell_size*x-5,padding_up+cell_size*y-5,10,10};
    SDL_RenderFillRect(renderer, &rect);
}

void GoGame::DrawBestMoveMarker(int x, int y)
{
    SDL_Color green = {0,255,0,255};

    SDL_SetRenderDrawColor(renderer,green.r,green.g,green.b,green.a);
    SDL_Rect rect = {padding_left+cell_size*x-5,padding_up+cell_size*y-5,10,10};
    SDL_RenderFillRect(renderer, &rect);
}

void GoGame::PlayMove(int mouse_x, int mouse_y)
{
    mouse_x -= padding_left;
    mouse_y -= padding_up;

    int x_rem = mouse_x % cell_size;
    int y_rem = mouse_y % cell_size;

    mouse_x /= cell_size;
    mouse_y /= cell_size;

    if(x_rem > cell_size/2) mouse_x++;
    if(y_rem > cell_size/2) mouse_y++;

    board.MakeMove({mouse_x,mouse_y});
}

void GoGame::HandleInput(bool *quit, bool *update_screen)
{

    SDL_Event event;
    while(SDL_PollEvent(&event))
    {
        switch(event.type)
        {
            case SDL_MOUSEBUTTONDOWN:
                if(event.button.button == SDL_BUTTON_LEFT)
                {
                    int x,y;
                    SDL_GetMouseState(&x,&y); 

                    if(mode == PLAY_BOTH_MODE || board.GetSideToMove() == player_side)
                    {
                        PlayMove(x,y);
                        *update_screen = true;
                    }
                }
                break;
            case SDL_KEYDOWN:
                if(event.key.keysym.sym == SDLK_p)
                {
                    board.Pass(); 
                }
                else if(event.key.keysym.sym == SDLK_u)
                {
                    board.UndoLastMove(); 

                    if(mode == AI_MODE)
                        board.UndoLastMove(); 

                    *update_screen = true;
                }
                else if(event.key.keysym.sym == SDLK_e)
                {
                    if(mode == PLAY_BOTH_MODE)
                    {
                        Coordinate best_move;

                        bool got_move = ai.GetBestMove(&board,&best_move);
                        if(got_move)
                        {
                            best_move_marker = best_move;
                        }

                        *update_screen = true;
                    }
                }
                break;
            case SDL_QUIT:
                *quit = true;
                break;
        }
    }
}

void GoGame::Draw()
{
    SDL_SetRenderDrawColor(renderer,222,232,158,255);
    SDL_RenderClear(renderer);

    SDL_SetRenderDrawColor(renderer,0,0,0,255);
    for(int x = 0; x < 9; ++x)
        SDL_RenderDrawLine(renderer,padding_left+x*cell_size,padding_up,padding_left+x*cell_size,padding_up+8*cell_size);

    for(int y = 0; y < 9; ++y)
        SDL_RenderDrawLine(renderer,padding_left,padding_up+y*cell_size,padding_left+8*cell_size,padding_up+y*cell_size);

    for(int y = 0; y < 9; ++y)
    {
        for(int x = 0; x < 9; ++x)
        {
            int value = board.GetTerritory(x,y);
            if(value != EMPTY && value != COLOR_NONE)
                DrawTerritoryMarker(x,y,value);
        }
    }

    if(best_move_marker.x != -1)
    {
        DrawBestMoveMarker(best_move_marker.x,best_move_marker.y);
    }

    for(int y = 0; y < 9; ++y)
    {
        for(int x = 0; x < 9; ++x)
        {
            int value = board.GetStone(x,y);
            if(value != EMPTY)
                DrawStone(x,y,value);
        }
    }

    SDL_RenderPresent(renderer);
}

void GoGame::Run(int play_mode, int side, int rules)
{
    this->mode = play_mode; 
    this->player_side = side;
    this->scoring_rules = rules; 

    bool quit = false;
    bool update_screen = true;
    
    bool ai_pass = false;
    while(!quit)
    {
        if(board.EndGame())
        {
            board.PrintState();
            board.PrintEndScreen();
            break;
        }

        if(play_mode == AI_MODE && board.GetSideToMove() != player_side)
        {

            Coordinate best_move;
            bool got_move = ai.GetBestMove(&board, &best_move);
            if(!got_move)
            {
                std::cout<<"Pass\n";
                board.Pass();
            }
            else
            {
                board.MakeMove(best_move);
            }

            update_screen = true;
        }


        HandleInput(&quit,&update_screen);
            
        if(update_screen)
        {

            board.CalculateScore(rules);
            Draw();
            update_screen = false;
            //board.PrintState();
            //std::cout<<"Evaluation: "<<board.Evaluate()<<"\n";

            std::cout<<std::endl;
            board.PrintState();
            if(board.GetMovesPlayed() != 0)
            {
                std::cout<<"Influence: \n";
                board.PrintEvalBoard();
            }
            std::cout<<std::endl;
            best_move_marker = {-1,-1};
        }
    }
}
