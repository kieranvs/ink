#include "lexer.h"

#include "utils.h"

#include <cstring>

char Lexer::peek()
{
	if (index >= input.size())
	{
		internal_error("Lexer read past end of input data");
	}

	return input[index];
}

char Lexer::get()
{
	if (index >= input.size())
	{
		internal_error("Lexer read past end of input data");
	}

	char ret = input[index];

	index += 1;
	update_line_col(ret);

	return ret;
}

bool Lexer::get_if(char c)
{
	if (input[index] == c)
	{
		index += 1;
		update_line_col(c);

		return true;
	}
	else
		return false;
}

bool Lexer::get_if(const char* pattern)
{
	int pattern_length = strlen(pattern);
	if (input.size() < index + pattern_length) return false;

	for (int i = 0; i < pattern_length; i++)
		if (input[index + i] != pattern[i])
			return false;

	for (int i = 0; i < pattern_length; i++)
		get();

	return true;
}

bool Lexer::has_more() const
{
	return input.size() > index;
}

bool Lexer::next_matches(const char* pattern) const
{
	int pattern_length = strlen(pattern);
	if (input.size() < index + pattern_length) return false;

	for (int i = 0; i < pattern_length; i++)
		if (input[index + i] != pattern[i])
			return false;

	return true;
}

void Lexer::update_line_col(char c)
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

bool valid_ident_start_char(char c)
{
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c == '_');
}

bool valid_ident_char(char c)
{
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || (c == '_');
}

void lex(std::vector<Token>& tokens, Lexer& lexer)
{
	while (lexer.has_more())
	{
		// Patterns which don't produce tokens first
		if (std::isspace(lexer.peek()))
		{
			lexer.get();
			continue;
		}
		else if (lexer.next_matches("//"))
		{
			while (lexer.has_more() && lexer.peek() != '\n')
				lexer.get();
			continue;
		}

		// Patterns which do produce tokens next
		tokens.emplace_back();
		Token& new_token = tokens.back();
		new_token.start_line = lexer.current_line;
		new_token.start_col = lexer.current_col;
		new_token.end_line = lexer.current_line;
		new_token.end_col = lexer.current_col;

		if (std::isdigit(lexer.peek()))
		{
			auto literal_string = lexer.get_while([](char c) { return std::isdigit(c); });

			int x = std::stoi(literal_string);
			new_token.type = TokenType::LiteralInteger;
			new_token.data_int = x;
		}
		else if (valid_ident_start_char(lexer.peek()))
		{
			auto identifier_string = lexer.get_while(valid_ident_char);

			if (identifier_string == "fn")
				new_token.type = TokenType::KeywordFunctionDecl;
			else if (identifier_string == "return")
				new_token.type = TokenType::KeywordReturn;
			else if (identifier_string == "true")
			{
				new_token.type = TokenType::LiteralBool;
				new_token.data_bool = true;
			}
			else if (identifier_string == "false")
			{
				new_token.type = TokenType::LiteralBool;
				new_token.data_bool = false;
			}
			else
			{
				new_token.type = TokenType::Identifier;
				new_token.data_str = identifier_string;
			}
		}
		else if (lexer.get_if(">="))
			new_token.type = TokenType::CompareGreaterEqual;
		else if (lexer.get_if("<="))
			new_token.type = TokenType::CompareLessEqual;
		else if (lexer.get_if("=="))
			new_token.type = TokenType::CompareEqual;
		else if (lexer.get_if("!="))
			new_token.type = TokenType::CompareNotEqual;
		else if (lexer.get_if('>'))
			new_token.type = TokenType::CompareGreater;
		else if (lexer.get_if('<'))
			new_token.type = TokenType::CompareLess;
		else if (lexer.get_if('*'))
			new_token.type = TokenType::OperatorMultiply;
		else if (lexer.get_if('+'))
			new_token.type = TokenType::OperatorPlus;
		else if (lexer.get_if('='))
			new_token.type = TokenType::Assign;
		else if (lexer.get_if(';'))
			new_token.type = TokenType::StatementEnd;
		else if (lexer.get_if('('))
			new_token.type = TokenType::ParenthesisLeft;
		else if (lexer.get_if(')'))
			new_token.type = TokenType::ParenthesisRight;
		else if (lexer.get_if('{'))
			new_token.type = TokenType::BraceLeft;
		else if (lexer.get_if('}'))
			new_token.type = TokenType::BraceRight;
		else if (lexer.get_if(','))
			new_token.type = TokenType::Comma;
		else if (lexer.get_if(':'))
			new_token.type = TokenType::Colon;
		else
			log_error(new_token, "Unrecognised token");

		new_token.end_line = lexer.current_line;
		new_token.end_col = lexer.current_col;
	}
}
