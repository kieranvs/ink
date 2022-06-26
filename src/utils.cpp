#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <fstream>

#include <execinfo.h>
#include <dlfcn.h>
#include <cxxabi.h>

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

void print_stack_trace()
{
	constexpr size_t max_frames = 64;
	void* callstack[max_frames];
	
	int num_frames = backtrace(callstack, max_frames);
	char** symbols = backtrace_symbols(callstack, num_frames);

	for (int i = 1; i < num_frames; i++)
	{
		Dl_info info;
		if (dladdr(callstack[i], &info))
		{
			if (info.dli_sname == nullptr)
			{
				printf("  %s\n", symbols[i]);
				continue;
			}

			int status;
			char* demangled = abi::__cxa_demangle(info.dli_sname, nullptr, 0, &status);

			printf("  %s\n", status == 0 ? demangled : info.dli_sname);
			free(demangled);
		}
		else
			printf("  %s\n", symbols[i]);
	}
	
	free(symbols);
	
	if (num_frames == max_frames)
		printf("[truncated]\n");
}