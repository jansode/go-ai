#ifndef GO_GAME_H 
#define GO_GAME_H 

#include "board.h"
#include "ai.h"
#include <SDL2/SDL.h>

enum Mode
{
    PLAY_BOTH_MODE,
    AI_MODE,

    NUM_MODES,
    MODE_NONE
};

class GoGame 
{
public:
    GoGame();
    ~GoGame();
    bool Init(int window_width, int window_height);
    void Run(int play_mode, int side = BLACK, int rules = JAPANESE_RULES);

    void DrawCircle(int radius, int x, int y, const SDL_Color& color);
    void DrawStone(int x, int y, int color);
    void DrawTerritoryMarker(int x, int y, int color);
    void DrawBestMoveMarker(int x, int y);

    void PlayMove(int mouse_x, int mouse_y);

    void HandleInput(bool *quit, bool *update_screen);
    void Draw();

private:
    SDL_Window *window = nullptr;
    SDL_Renderer *renderer = nullptr;

    int window_width = 500;
    int window_height = 500;

    int padding_left = 40; 
    int padding_right = 40; 
    int padding_down = 20; 
    int padding_up = 40; 

    int cell_size = 50;

    int mode;
    int player_side;
    int scoring_rules;

    Coordinate best_move_marker = {-1,-1};

    Board board;
    Ai ai;

};

#endif
