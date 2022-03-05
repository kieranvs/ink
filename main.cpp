#include "ast.h"
#include "lexer.h"
#include "parser.h"
#include "codegen.h"
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

    std::stringstream sstr;
    sstr << input_file.rdbuf();
    std::string str = sstr.str();

	std::vector<Token> tokens;
	Lexer lexer(str);
	lex(tokens, lexer);

	Parser parser(tokens);

	SymbolTable symbol_table;
	
	parse_top_level(parser, symbol_table);

	FILE* file_ptr = fopen("test.asm","w");
	if (file_ptr == nullptr)
		fail("Cannot open test.asm for writing!\n");

	codegen(symbol_table, file_ptr);

	fclose(file_ptr);
}
