#include "lexer.h"

#include "utils.h"

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
		if (std::isspace(lexer.peek()))
		{
			lexer.get();
			continue;
		}

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
			{
				new_token.type = TokenType::KeywordFunctionDecl;
			}
			else
			{
				new_token.type = TokenType::Identifier;
				new_token.data_str = identifier_string;
			}
		}
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
		else
			log_error(new_token, "Unrecognised token");

		new_token.end_line = lexer.current_line;
		new_token.end_col = lexer.current_col;
	}
}
