#include "utils.h"

#include <stdio.h>
#include <stdlib.h>

const char* file_source = nullptr;

void set_current_file(const char* src)
{
	file_source = src;
}

void internal_error(const char* message)
{
	printf("%s\n", message);
	exit(1);
}

void log_error(const char* message)
{
	printf("%s\n", message);
	exit(101);
}

void log_error(const Token& token, const char* message)
{
	size_t current_line = 1;
	size_t current_col = 1;
	const char* current_ptr = file_source;

	while (current_ptr && current_line != token.start_line)
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

	for (int i = 0; i < token.start_col - 1; i++)
		printf(" ");
	printf("^");

	if (token.start_line == token.end_line && token.end_col > token.start_col + 1)
	{
		for (int i = 0; i < token.end_col - token.start_col - 2; i++)
			printf("~");
		printf("^");
	}
	printf("\n");

	printf("%s\n", message);
	exit(101);
}