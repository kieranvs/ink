#pragma once

#include <vector>
#include <string>

enum class TokenType
{
	KeywordFunctionDecl,
	KeywordReturn,
	ParenthesisLeft,
	ParenthesisRight,
	BraceLeft,
	BraceRight,
	LiteralInteger,
	LiteralBool,
	OperatorPlus,
	OperatorMultiply,
	CompareGreater,
	CompareGreaterEqual,
	CompareLess,
	CompareLessEqual,
	CompareEqual,
	CompareNotEqual,
	Identifier,
	Assign,
	StatementEnd,
	Comma,
	Colon
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
	bool data_bool;
};

struct Lexer
{
	Lexer(const std::string& i) : input(i) {}

	char peek();
	char get();
	bool get_if(char c);
	bool get_if(const char* pattern);

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

	bool has_more() const;
	bool next_matches(const char* pattern) const;
	void update_line_col(char c);

	const std::string& input;
	size_t index = 0;
	size_t current_line = 1;
	size_t current_col = 1;
};

void lex(std::vector<Token>& tokens, Lexer& lexer);