#include "lexer.h"

void lex(std::vector<Token>& tokens, Lexer& lexer)
{
	while (lexer.has_more())
	{
		if (std::isdigit(lexer.peek()))
		{
			auto literal_string = lexer.get_while([](char c) { return std::isdigit(c); });

			int x = std::stoi(literal_string);
			tokens.emplace_back();
			tokens.back().type = TokenType::LiteralInteger;
			tokens.back().data_int = x;
		}
		else if (lexer.get_if('*'))
		{
			tokens.emplace_back();
			tokens.back().type = TokenType::OperatorMultiply;
		}
		else if (lexer.get_if('+'))
		{
			tokens.emplace_back();
			tokens.back().type = TokenType::OperatorPlus;
		}
		else if (std::isalpha(lexer.peek()))
		{
			auto identifier_string = lexer.get_while([](char c) { return std::isalnum(c); });

			tokens.emplace_back();
			tokens.back().type = TokenType::Identifier;
			tokens.back().data_str = identifier_string;
		}
		else if (lexer.get_if('='))
		{
			tokens.emplace_back();
			tokens.back().type = TokenType::Assign;
		}
		else if (lexer.get_if(';'))
		{
			tokens.emplace_back();
			tokens.back().type = TokenType::StatementEnd;
		}
		else
			lexer.get();
	}
}
