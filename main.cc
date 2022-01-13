#include <iostream>
#include "go_game.h"

int main()
{
    GoGame go;
    if(!go.Init(500,500))
        return 1;

    go.Run(AI_MODE);

    /*
    bool playing = true;
    while(playing)
    {
        board.Print();
        board.PrintTerritory();
        std::cout<<kSidesString[board.GetSideToMove()]<<" to move:\n";

        int x,y;
        std::cin>>x>>y;

        board.MakeMove({x,y});
        board.CalculateScore(JAPANESE_RULES);
    }
    */
}
