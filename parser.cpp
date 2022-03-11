#include "parser.h"

#include "utils.h"

#include <stack>

const Token& Parser::peek()
{
	if (index >= input.size())
	{
		internal_error("Parser read past end of input data");
	}

	return input[index];
}

const Token& Parser::get()
{
	if (index >= input.size())
	{
		internal_error("Parser read past end of input data");
	}

	const Token& ret = input[index];
	index += 1;
	return ret;
}

const Token& Parser::get_if(TokenType type, const char* error_message)
{
	if (index >= input.size())
		internal_error("Parser read past end of input data");

	const Token& ret = input[index];
	index += 1;

	if (ret.type != type)
		log_error(ret, error_message);

	return ret;
}

size_t parse_expression(Parser& parser, Ast& ast, SymbolTable& symbol_table, size_t scope_index)
{
	std::stack<size_t> expr_nodes;
	std::stack<Token> operators;
	size_t index = 0;

	auto priority = [](const Token& t)
	{
		if (t.type == TokenType::OperatorPlus)
			return 1;
		else if (t.type == TokenType::OperatorMultiply)
			return 2;
		else return 0;
	};

	auto apply_op = [&operators, &expr_nodes, &ast]()
	{
		auto& top = operators.top();

		if (expr_nodes.size() < 2)
			log_error("Missing operand for binary operator");
		
		size_t expr0 = expr_nodes.top();
		expr_nodes.pop();
		size_t expr1 = expr_nodes.top();
		expr_nodes.pop();

		size_t node;
		if (top.type == TokenType::OperatorPlus)
			node = ast.make(AstNodeType::BinOpAdd);
		else
			node = ast.make(AstNodeType::BinOpMul);

		ast[node].child0 = expr0;
		ast[node].child1 = expr1;

		expr_nodes.push(node);
		operators.pop();
	};

	while (parser.has_more() && !parser.next_is(TokenType::StatementEnd))
	{
		auto& next_token = parser.get();
		if (next_token.type == TokenType::LiteralInteger)
		{
			auto node = ast.make(AstNodeType::LiteralInt);
			ast[node].data_literal_int.value = next_token.data_int;
			expr_nodes.push(node);
		}
		else if (next_token.type == TokenType::OperatorPlus || next_token.type == TokenType::OperatorMultiply)
		{
			while (!operators.empty() && priority(operators.top()) > priority(next_token)) // or they are the same and next_token is left assoc
			{
				apply_op();
			}

			operators.push(next_token);
		}
		else if (next_token.type == TokenType::Identifier)
		{
			auto& scope = symbol_table.scopes[scope_index];

			if (auto variable = scope.find_variable(next_token.data_str, false))
			{
				auto node = ast.make(AstNodeType::Variable);
				ast[node].data_variable.offset = variable->stack_offset;
				expr_nodes.push(node);
			}
			else if (auto function = symbol_table.find_function(next_token.data_str))
			{
				parser.get_if(TokenType::ParenthesisLeft, "Missing argument list");
				parser.get_if(TokenType::ParenthesisRight, "Missing argument list");
				auto node = ast.make(AstNodeType::FunctionCall);
				ast[node].data_function_call.function_index = function.value();
				expr_nodes.push(node);
			}
			else
			{
				log_error(next_token, "Undefined variable");
			}
		}
		else
		{
			log_error(next_token, "Unexpected token in expression");
		}
	}

	if (!parser.has_more())
		log_error("Unexpected end of expression");

	auto& end_of_expr_token = parser.peek();

	while (parser.next_is(TokenType::StatementEnd)) parser.get();

	while (!operators.empty())
		apply_op();

	if (expr_nodes.empty())
		log_error(end_of_expr_token, "Empty expression");

	return expr_nodes.top();
}

size_t parse_statement(Parser& parser, Ast& ast, SymbolTable& symbol_table, size_t scope_index)
{
	// Assignment to existing variable
	if (parser.next_is(TokenType::Identifier, TokenType::Assign))
	{
		auto& ident_token = parser.get();
		auto& assign_token = parser.get();

		auto expr_node = parse_expression(parser, ast, symbol_table, scope_index);
		size_t var_node = ast.make(AstNodeType::Variable);

		auto& scope = symbol_table.scopes[scope_index];
		auto variable = scope.find_variable(ident_token.data_str, false);
		if (!variable)
			log_error(ident_token, "Undefined variable");

		ast[var_node].data_variable.offset = variable->stack_offset;

		size_t assign_node = ast.make(AstNodeType::Assignment);
		ast[assign_node].child0 = var_node;
		ast[assign_node].child1 = expr_node;

		return assign_node;
	}
	// Assignment to new variable
	else if (parser.next_is(TokenType::Identifier, TokenType::Identifier, TokenType::Assign))
	{
		auto& type_token = parser.get();
		auto& ident_token = parser.get();
		auto& assign_token = parser.get();

		// For now, only support int type
		if (type_token.data_str != "int")
			log_error(type_token, "Unknown type");

		auto expr_node = parse_expression(parser, ast, symbol_table, scope_index);
		size_t var_node = ast.make(AstNodeType::Variable);

		auto& scope = symbol_table.scopes[scope_index];
		auto variable = scope.find_variable(ident_token.data_str, true);
		if (!variable)
			internal_error("Failed to create new variable");

		ast[var_node].data_variable.offset = variable->stack_offset;

		size_t assign_node = ast.make(AstNodeType::Assignment);
		ast[assign_node].child0 = var_node;
		ast[assign_node].child1 = expr_node;

		return assign_node;
	}
	// Return
	else if (parser.next_is(TokenType::KeywordReturn))
	{
		auto& return_token = parser.get();

		auto expr_node = parse_expression(parser, ast, symbol_table, scope_index);

		size_t return_node = ast.make(AstNodeType::Return);
		ast[return_node].child0 = expr_node;

		return return_node;
	}
	else
	{
		log_error(parser.peek(), "Invalid statement, expected variable or type");
		return 0;
	}
}

void parse_function(Parser& parser, SymbolTable& symbol_table)
{
	parser.get_if(TokenType::KeywordFunctionDecl, "Invalid function declaration");
	auto& func_ident_token = parser.get_if(TokenType::Identifier, "Expected function name");
	parser.get_if(TokenType::ParenthesisLeft, "Expected (");
	parser.get_if(TokenType::ParenthesisRight, "Expected )");
	parser.get_if(TokenType::BraceLeft, "Expected {");

	symbol_table.functions.emplace_back();
	Function& func = symbol_table.functions.back();
	func.name = func_ident_token.data_str;

	size_t func_node = func.ast.make(AstNodeType::FunctionDefinition);
	func.ast_node_root = func_node;

	symbol_table.scopes.emplace_back();
	size_t scope = symbol_table.scopes.size() - 1;

	if (parser.next_is(TokenType::BraceRight))
	{
		parser.get();
	}
	else if (parser.has_more())
	{
		func.ast[func_node].next = parse_statement(parser, func.ast, symbol_table, scope);
		size_t prev_node = func.ast[func_node].next.value();
		while (parser.has_more())
		{
			if (parser.next_is(TokenType::BraceRight))
			{
				parser.get();
				break;
			}

			size_t st = parse_statement(parser, func.ast, symbol_table, scope);
			func.ast[prev_node].next = st;
			prev_node = st;
		}
	}

	int stack_size = 0;
	for (auto& v : symbol_table.scopes[scope].local_variables)
	{
		if (stack_size < v.stack_offset + 4)
			stack_size = v.stack_offset + 4;
	}

	if (stack_size % 16 != 0)
		stack_size = ((stack_size / 16) + 1) * 16;

	func.ast[func_node].data_function_definition.stack_size = stack_size;
}

void parse_top_level(Parser& parser, SymbolTable& symbol_table)
{
	while (parser.has_more())
	{
		if (parser.next_is(TokenType::KeywordFunctionDecl))
			parse_function(parser, symbol_table);
		else
			log_error(parser.peek(), "Unexpected token at top level");
	}
}