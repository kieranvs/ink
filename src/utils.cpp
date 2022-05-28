#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <fstream>

int exec_process(const char* cmd, std::string& output)
{
	char buffer[128];

	FILE* pipe = popen(cmd, "r");
	if (!pipe) throw std::runtime_error("popen() failed!");
	try
	{
		while (fgets(buffer, sizeof buffer, pipe) != NULL)
			output += buffer;
	}
	catch (...)
	{
		pclose(pipe);
		throw;
	}

	int retval = pclose(pipe);
	if (WIFEXITED(retval) != 0)
		return WEXITSTATUS(retval);
	else
		return -1;
}
