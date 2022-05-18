#include "Hub.h"

int main()
{
	Hub::init();
	Hub::loop();
	Hub::release();

	return 0;
}