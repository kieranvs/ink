#pragma once

#include "lexer.h"
#include "ast.h"

struct Parser
{
	Parser(const std::vector<Token>& i) : input(i) {}

	const Token& get()
	{
		if (index >= input.size())
		{
			fail("Parser read past end of input data\n");
		}

		const Token& ret = input[index];
		index += 1;
		return ret;
	}

	const Token& get_if(TokenType type, const char* error_message)
	{
		if (index >= input.size())
			fail("Parser read past end of input data\n");

		const Token& ret = input[index];
		index += 1;

		if (ret.type != type)
			fail(error_message);

		return ret;
	}

	bool next_is(TokenType type) const
	{
		return has_more() && input[index].type == type;
	}

	bool has_more() const
	{
		return input.size() > index;
	}

	const std::vector<Token>& input;
	size_t index = 0;
};

void parse_top_level(Parser& parser, SymbolTable& symbol_table);