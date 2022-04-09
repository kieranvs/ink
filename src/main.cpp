#include "ast.h"
#include "lexer.h"
#include "parser.h"
#include "codegen.h"
#include "typecheck.h"
#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <cctype>
#include <cstring>
#include <unistd.h>

#include <vector>
#include <optional>
#include <string>
#include <fstream>
#include <sstream>

struct CommandLineOptions
{
	std::optional<std::string> input_file;
	std::optional<std::string> output_binary;
	std::optional<std::string> output_asm;
	std::optional<std::string> output_debug_data;
};

void fail_usage(const char* executable_name)
{
	printf("Usage: %s input_file [options]\n", executable_name);
	printf("   or: %s [options] input_file\n", executable_name);
	printf("\n Options:\n");
	printf("  -o <file>                    Output binary name\n");
	printf("  -a <file>                    Output assembly file\n");
	printf("  --dump-symbols <file>        Dump debug information\n");
	printf("  -h                           Print this message\n");
	printf("\n");
	exit(1);
}

void fail_custom(const char* message)
{
	printf("%s", message);
	exit(1);
}

CommandLineOptions parse_arguments(int argc, char** argv)
{
	CommandLineOptions options;

	if (argc < 2)
		fail_custom("No input file provided!\n");

	int current_arg = 1;
	auto do_flag = [&current_arg, argc, argv](std::optional<std::string>& str)
	{
		if (current_arg + 1 >= argc)
			fail_usage(argv[0]);

		if (str.has_value())
			fail_custom("Flag set more than once!\n");

		str = argv[current_arg + 1];
		current_arg += 2;
	};

	while (current_arg != argc)
	{
		if (strcmp(argv[current_arg], "-o") == 0)
			do_flag(options.output_binary);
		else if (strcmp(argv[current_arg], "-a") == 0)
			do_flag(options.output_asm);
		else if (strcmp(argv[current_arg], "--dump-symbols") == 0)
			do_flag(options.output_debug_data);
		else if (strcmp(argv[current_arg], "-h") == 0)
			fail_usage(argv[0]);
		else if (argv[current_arg][0] == '-')
			fail_usage(argv[0]);
		else
		{
			if (options.input_file.has_value())
				fail_custom("Input file set more than once!\n");
			options.input_file = argv[current_arg];
			current_arg += 1;
		}
	}

	if (!options.input_file.has_value())
		fail_custom("No input file provided!\n");

	if (!options.output_binary.has_value())
		options.output_binary = options.input_file.value() + "-bin";

	return options;
}

int main(int argc, char** argv)
{
	CommandLineOptions options = parse_arguments(argc, argv);

	std::ifstream input_file;
	input_file.open(options.input_file.value());
	if (input_file.fail())
	{
		printf("Failed to open input file %s\n", options.input_file.value().c_str());

		internal_error("IO failure");
	}

    std::stringstream sstr;
    sstr << input_file.rdbuf();
    std::string str = sstr.str();

    set_current_file(str.data());

	std::vector<Token> tokens;
	Lexer lexer(str);

	lex(tokens, lexer);

	Parser parser(tokens);

	SymbolTable symbol_table;

	{
		symbol_table.types.emplace_back();
		auto& type = symbol_table.types.back();
		type.intrinsic = true;
		type.name = "int";
		type.data_size = 8;
	}

	{
		symbol_table.types.emplace_back();
		auto& type = symbol_table.types.back();
		type.intrinsic = true;
		type.name = "bool";
		type.data_size = 1;
	}

	{
		symbol_table.types.emplace_back();
		auto& type = symbol_table.types.back();
		type.intrinsic = true;
		type.name = "char";
		type.data_size = 1;
	}

	{
		symbol_table.scopes.emplace_back();
		auto scope_index = symbol_table.scopes.size() - 1;
		auto& scope = symbol_table.scopes.back();

		scope.local_variables.emplace_back();
		scope.local_variables.back().type_index = symbol_table.find_type("int").value();

		symbol_table.functions.emplace_back();
		auto& func = symbol_table.functions.back();
		func.scope = scope_index;
		func.name = "print_uint32";
		func.parameters.push_back(0);
		func.intrinsic = true;
	}

	{
		symbol_table.scopes.emplace_back();
		auto scope_index = symbol_table.scopes.size() - 1;
		auto& scope = symbol_table.scopes.back();

		scope.local_variables.emplace_back();
		scope.local_variables.back().type_index = symbol_table.find_type("bool").value();

		symbol_table.functions.emplace_back();
		auto& func = symbol_table.functions.back();
		func.scope = scope_index;
		func.name = "print_bool";
		func.parameters.push_back(0);
		func.intrinsic = true;
	}

	{
		symbol_table.scopes.emplace_back();
		auto scope_index = symbol_table.scopes.size() - 1;
		auto& scope = symbol_table.scopes.back();

		scope.local_variables.emplace_back();
		scope.local_variables.back().type_index = symbol_table.find_type("char").value();

		symbol_table.functions.emplace_back();
		auto& func = symbol_table.functions.back();
		func.scope = scope_index;
		func.name = "print_char";
		func.parameters.push_back(0);
		func.intrinsic = true;
	}
	
	parse_top_level(parser, symbol_table);

	if (options.output_debug_data.has_value())
	{
		FILE* debug_output_file = fopen(options.output_debug_data.value().c_str(), "w");
		if (debug_output_file == nullptr)
		{
			printf("Failed to open %s for writing!\n", options.output_debug_data.value().c_str());

			internal_error("IO failure");
		}

		// Dump out symbol table for debugging:
		for (auto& func : symbol_table.functions)
		{
			if (func.intrinsic) continue;

			fprintf(debug_output_file, "Function %s\n", func.name.c_str());
			fprintf(debug_output_file, "Parameters: ");
			for (auto x : func.parameters)
				fprintf(debug_output_file, "%zd", x);
			fprintf(debug_output_file, "\n");

			fprintf(debug_output_file, "Variables: ");
			for (auto& variable : symbol_table.scopes[func.scope].local_variables)
			{
				fprintf(debug_output_file, "%s %d\n", variable.name.c_str(), variable.type_index);
			}
			fprintf(debug_output_file, "\n");

			fprintf(debug_output_file, "Scope: %d\n", func.scope);
			dump_ast(debug_output_file, symbol_table, func.ast);
		}

		fclose(debug_output_file);
	}

	type_check(symbol_table);

	std::string asm_file_name;
	bool should_delete_asm_file = false;
	if (options.output_asm.has_value())
		asm_file_name = options.output_asm.value();
	else
	{
		asm_file_name = std::tmpnam(nullptr);
		add_file_to_delete_at_exit(asm_file_name);
	}
	FILE* asm_file = fopen(asm_file_name.c_str(), "w");

	if (asm_file == nullptr)
	{
		printf("Failed to open %s for writing!\n", asm_file_name.c_str());

		internal_error("IO failure");
	}

	codegen(symbol_table, asm_file);

	fclose(asm_file);

	std::string obj_file_name = std::tmpnam(nullptr);

	// Run the assembler
	{
		std::string assembler_output;
		char assembler_command[512];

		const char* binary_format;
		if (get_platform() == Platform::Linux)
			binary_format = "elf64";
		else if (get_platform() == Platform::MacOS)
			binary_format = "macho64";

		snprintf(assembler_command, 512, "yasm -f %s %s -o %s", binary_format, asm_file_name.c_str(), obj_file_name.c_str());
		int assembler_error = exec_process(assembler_command, assembler_output);
		if (assembler_error != 0)
		{
			printf("Assembler returned %d\n", assembler_error);
			printf("Assembler output:\n%s\n", assembler_output.c_str());

			internal_error("Assembler failed");
		}
	}

	// Run the linker
	{
		std::string linker_output;
		char linker_command[512];
		const char* extra_str = "";
		if (get_platform() == Platform::MacOS)
			extra_str = "-static ";
		snprintf(linker_command, 512, "ld %s-o %s %s", extra_str, options.output_binary.value().c_str(), obj_file_name.c_str());
		int linker_error = exec_process(linker_command, linker_output);
		if (linker_error != 0)
		{
			printf("Linker returned %d\n", linker_error);
			printf("Linker output:\n%s\n", linker_output.c_str());

			internal_error("Linker failed");
		}

		add_file_to_delete_at_exit(obj_file_name);
	}

	delete_exit_files();
}
