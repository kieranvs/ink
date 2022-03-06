#pragma once

#include <vector>
#include <string>

enum class TokenType
{
	KeywordFunctionDecl,
	ParenthesisLeft,
	ParenthesisRight,
	BraceLeft,
	BraceRight,
	LiteralInteger,
	OperatorPlus,
	OperatorMultiply,
	Identifier,
	Assign,
	StatementEnd
};

struct Token
{
	TokenType type;

	int start_line;
	int start_col;
	int end_line;
	int end_col;

	int data_int;
	std::string data_str;
};

struct Lexer
{
	Lexer(const std::string& i) : input(i) {}

	char peek();
	char get();
	bool get_if(char c);

	template <typename FuncType>
	std::string get_while(FuncType condition)
	{
		auto start_index = index;
		while (condition(input[index]))
		{
			update_line_col(input[index]);
			index += 1;
		}
		return input.substr(start_index, index - start_index);
	}

	bool has_more() const
	{
		return input.size() > index;
	}

	void update_line_col(char c)
	{
		if (c == '\n')
		{
			current_line += 1;
			current_col = 1;
		}
		else if (c == '\t')
		{
			current_col += 8;
		}
		else
			current_col += 1;
	}

	const std::string& input;
	size_t index = 0;
	size_t current_line = 1;
	size_t current_col = 1;
};

void lex(std::vector<Token>& tokens, Lexer& lexer);