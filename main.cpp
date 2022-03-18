#include "ast.h"
#include "lexer.h"
#include "parser.h"
#include "codegen.h"
#include "typecheck.h"
#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <cctype>

#include <vector>
#include <optional>
#include <string>
#include <fstream>
#include <sstream>

int main(int argc, char** argv)
{
	std::ifstream input_file;
    input_file.open(argv[1]);
    if (input_file.fail())
    {
		internal_error("Cannot open input file");
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

	symbol_table.functions.emplace_back();
	auto& func = symbol_table.functions.back();
	func.intrinsic = true;
	func.parameters.push_back(0);
	func.name = "print_uint32";

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
	
	parse_top_level(parser, symbol_table);

	type_check(symbol_table);

	FILE* file_ptr = fopen("test.asm","w");
	if (file_ptr == nullptr)
		internal_error("Cannot open test.asm for writing!");

	codegen(symbol_table, file_ptr);

	fclose(file_ptr);
}
