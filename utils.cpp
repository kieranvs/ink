#include "utils.h"

#include <stdio.h>
#include <stdlib.h>

void fail(const char* message)
{
	printf(message);
	exit(-1);
}