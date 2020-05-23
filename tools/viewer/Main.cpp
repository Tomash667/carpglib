#include "Pch.h"
#include <AppEntry.h>
#include "Viewer.h"

int AppEntry(char*)
{
	Viewer game;
	return game.Start() ? 0 : 1;
}
