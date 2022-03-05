#include "lexer.h"

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
			fail("Unrecognised token\n");
	}
}
