#include "errors.h"

#include "file_table.h"
#include "utils.h"

#include <stddef.h>
#include <stdio.h>

#define CONSOLE_NRM  "\x1B[0m"
#define CONSOLE_RED  "\x1B[31m"
#define CONSOLE_GRN  "\x1B[32m"
#define CONSOLE_YEL  "\x1B[33m"
#define CONSOLE_BLU  "\x1B[34m"
#define CONSOLE_MAG  "\x1B[35m"
#define CONSOLE_CYN  "\x1B[36m"
#define CONSOLE_WHT  "\x1B[37m"

std::vector<std::string> files_to_delete_at_exit;

struct NoteData
{
	TypeAnnotation ta;
	SymbolTable* symbol_table;
	const char* label;
};
std::vector<NoteData> notes_to_print;

[[noreturn]] void internal_error(const char* message)
{
	printf("%sInternal compiler error: %s%s\n\n", CONSOLE_RED, CONSOLE_NRM, message);
	printf("Stack trace:\n%s", CONSOLE_BLU);
	print_stack_trace();
	printf("%s\n", CONSOLE_NRM);
	delete_exit_files();

	exit(1);
}

[[noreturn]] void log_general_error(const char* message)
{
	delete_exit_files();

	printf("%s\n", message);
	exit(101);
}

void log_error(const SourceLocation& location, const char* message)
{
	size_t current_line = 1;
	size_t current_col = 1;
	const char* current_ptr = file_table[location.source_file].contents.c_str();

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

	printf("%sError: %s%s\n", CONSOLE_RED, message, CONSOLE_NRM);
	printf("%s:%d:\n", file_table[location.source_file].name.c_str(), location.start_line);

	printf("%s", CONSOLE_BLU);
	while (*current_ptr != '\n' && *current_ptr != '\0')
	{
		if (current_col == location.start_col)
			printf("%s", CONSOLE_RED);
		else if (current_col == location.end_col)
			printf("%s", CONSOLE_BLU);

		printf("%c", *current_ptr);
		current_col += *current_ptr == '\t' ? 8 : 1;
		current_ptr += 1;
	}
	printf("%s\n", CONSOLE_NRM);

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

	for (auto& note : notes_to_print)
	{
		printf("Note: %s has type ", note.label);
		if (note.ta.special)
		{
			if (note.ta.type_index == TypeAnnotation::special_type_index_literal_int)
				printf("Literal Integer", note.ta.type_index);
			else if (note.ta.type_index == TypeAnnotation::special_type_index_literal_bool)
				printf("Literal Bool", note.ta.type_index);
			else if (note.ta.type_index == TypeAnnotation::special_type_index_literal_char)
				printf("Literal Char", note.ta.type_index);
			else if (note.ta.type_index == TypeAnnotation::special_type_index_literal_float)
				printf("Literal Float", note.ta.type_index);
			else
				printf("Special Type %d", note.ta.type_index);
		}
		else
			pretty_print_type(stdout, *note.symbol_table, note.ta.type_index);
		printf("\n");
	}
	notes_to_print.clear();
}

[[noreturn]] void log_error(const Token& token, const char* message)
{
	delete_exit_files();

	log_error(token.location, message);

	exit(101);
}

[[noreturn]] void log_error(const Type& type, const char* message)
{
	delete_exit_files();

	log_error(type.location, message);

	exit(101);
}

[[noreturn]] void log_error(const AstNode& node, const char* message)
{
	delete_exit_files();

	log_error(node.location, message);

	exit(101);
}

void log_note_type(const AstNode& node, SymbolTable& symbol_table, const char* label)
{
	log_note_type(node.type_annotation.value(), symbol_table, label);
}

void log_note_type(const TypeAnnotation& ta, SymbolTable& symbol_table, const char* label)
{
	auto& note = notes_to_print.emplace_back();
	note.ta = ta;
	note.symbol_table = &symbol_table;
	note.label = label;
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