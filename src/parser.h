#pragma once

#include "lexer.h"
#include "ast.h"

struct Parser
{
	Parser(const std::vector<Token>& i) : input(i) {}

	const Token& peek();
	const Token& get();
	const Token& get_if(TokenType type, const char* error_message);

	bool next_is(TokenType type) const
	{
		return has_more() && input[index].type == type;
	}

	// Template this
	bool next_is(TokenType t0, TokenType t1) const
	{
		return input.size() > index + 1 &&
			   input[index].type == t0 &&
			   input[index + 1].type == t1;
	}

	bool next_is(TokenType t0, TokenType t1, TokenType t2) const
	{
		return input.size() > index + 2 &&
			   input[index].type == t0 &&
			   input[index + 1].type == t1 &&
			   input[index + 2].type == t2;
	}

	bool has_more() const
	{
		return input.size() > index;
	}

	const std::vector<Token>& input;
	size_t index = 0;
};

void parse_top_level(Parser& parser, SymbolTable& symbol_table);