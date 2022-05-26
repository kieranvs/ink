#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <fstream>

const char* file_source = nullptr;

std::vector<std::string> files_to_delete_at_exit;

void set_current_file(const char* src)
{
	file_source = src;
}

[[noreturn]] void internal_error(const char* message)
{
	delete_exit_files();

	printf("%s\n", message);
	exit(1);
}

[[noreturn]] void log_general_error(const char* message)
{
	delete_exit_files();

	printf("%s\n", message);
	exit(101);
}

void log_error(const SourceLocation& location)
{
	size_t current_line = 1;
	size_t current_col = 1;
	const char* current_ptr = file_source;

	while (current_ptr && current_line != location.start_line)
	{
		if (*current_ptr == '\n')
		{
			current_line += 1;
			current_col = 1;
		}
		else
			current_col += 1;

		current_ptr += 1;
	}

	while (*current_ptr != '\n' && *current_ptr != '\0')
	{
		printf("%c", *current_ptr);
		current_ptr += 1;
	}
	printf("\n");

	for (int i = 0; i < location.start_col - 1; i++)
		printf(" ");
	printf("^");

	if (location.start_line == location.end_line && location.end_col > location.start_col + 1)
	{
		for (int i = 0; i < location.end_col - location.start_col - 2; i++)
			printf("~");
		printf("^");
	}
	printf("\n");
}

[[noreturn]] void log_error(const Token& token, const char* message)
{
	delete_exit_files();

	log_error(token.location);

	printf("%s\n", message);
	exit(101);
}

[[noreturn]] void log_error(const AstNode& node, const char* message)
{
	delete_exit_files();

	log_error(node.location);

	printf("%s\n", message);
	exit(101);
}

void add_file_to_delete_at_exit(const std::string& file)
{
	files_to_delete_at_exit.push_back(file);
}

void delete_exit_files()
{
	for (auto& file : files_to_delete_at_exit)
		if (remove(file.c_str()))
			printf("Failed to delete %s\n", file.c_str());
}

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
