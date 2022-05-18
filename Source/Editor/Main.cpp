#include "Editor.h"

int main()
{
	Editor::init();

	Editor::loop();

	Editor::release();

	return 0;
}