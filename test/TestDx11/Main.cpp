#include <CarpgLib.h>
#include <AppEntry.h>
#include "Game.h"

int AppEntry(char*)
{
	Game game;
	game.Start();

	return 0;
}
