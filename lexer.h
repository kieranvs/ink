#pragma once

#include <vector>
#include <string>

#include "utils.h"

enum class TokenType
{
	IntegerLiteral,
	Plus,
	Multiply,
	Identifier,
	Assign,
	StatementEnd
};

struct Token
{
	TokenType type;
	int data_int;
	std::string data_str;
};

struct Lexer
{
	Lexer(const std::string& i) : input(i) {}

	char peek()
	{
		if (index >= input.size())
		{
			fail("Lexer read past end of input data\n");
		}

		return input[index];
	}

	char get()
	{
		if (index >= input.size())
		{
			fail("Lexer read past end of input data\n");
		}

		char ret = input[index];
		index += 1;
		return ret;
	}

	template <typename FuncType>
	std::string get_while(FuncType condition)
	{
		auto start_index = index;
		while (condition(input[index])) index += 1;
		return input.substr(start_index, index - start_index);
	}

	bool get_if(char c)
	{
		if (input[index] == c)
		{
			index += 1;
			return true;
		}
		else
			return false;
	}

	bool has_more() const {
		return input.size() > index;
	}

	const std::string& input;
	size_t index = 0;
};

void lex(std::vector<Token>& tokens, Lexer& lexer);