#pragma once

#include <vector>
#include <string>

enum class TokenType
{
	KeywordFunctionDecl,
	KeywordReturn,
	KeywordIf,
	KeywordElse,
	KeywordWhile,
	KeywordFor,
	KeywordExternal,
	KeywordStruct,
	KeywordFunctionType,
	DirectiveLink,
	DirectiveLinkFramework,
	ParenthesisLeft,
	ParenthesisRight,
	BraceLeft,
	BraceRight,
	LiteralInteger,
	LiteralFloat,
	LiteralBool,
	LiteralChar,
	LiteralString,
	OperatorPlus,
	Asterisk,
	OperatorMinus,
	OperatorDivide,
	CompareGreater,
	CompareGreaterEqual,
	CompareLess,
	CompareLessEqual,
	CompareEqual,
	CompareNotEqual,
	LogicalOr,
	LogicalAnd,
	Identifier,
	Assign,
	StatementEnd,
	Comma,
	Colon,
	Ampersand,
	Period
};

struct SourceLocation
{
	const char* source_file;
	int start_line;
	int start_col;
	int end_line;
	int end_col;
};

struct Token
{
	TokenType type;
	SourceLocation location;

	int data_int;
	std::string data_str;
	bool data_bool;
	double data_float;
};

struct Lexer
{
	Lexer(const std::string& i, const char* file_name) : input(i), source_file(file_name) {}

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
	const char* source_file;
	size_t index = 0;
	size_t current_line = 1;
	size_t current_col = 1;
};

void lex(std::vector<Token>& tokens, Lexer& lexer);