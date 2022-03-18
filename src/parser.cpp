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

size_t parse_expression(Parser& parser, Ast& ast, SymbolTable& symbol_table, size_t scope_index, TokenType end_token)
{
	std::stack<size_t> expr_nodes;
	std::stack<Token> operators;
	size_t index = 0;

	auto priority = [](const Token& t)
	{
		if (t.type == TokenType::OperatorPlus) return 1;
		else if (t.type == TokenType::OperatorMultiply)	return 2;
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
		if (top.type == TokenType::OperatorPlus)          node = ast.make(AstNodeType::BinOpAdd);
		else if (top.type == TokenType::OperatorMultiply)    node = ast.make(AstNodeType::BinOpMul);
		else if (top.type == TokenType::CompareGreater)      node = ast.make(AstNodeType::BinCompGreater);
		else if (top.type == TokenType::CompareGreaterEqual) node = ast.make(AstNodeType::BinCompGreaterEqual);
		else if (top.type == TokenType::CompareLess)         node = ast.make(AstNodeType::BinCompLess);
		else if (top.type == TokenType::CompareLessEqual)    node = ast.make(AstNodeType::BinCompLessEqual);
		else if (top.type == TokenType::CompareEqual)        node = ast.make(AstNodeType::BinCompEqual);
		else if (top.type == TokenType::CompareNotEqual)     node = ast.make(AstNodeType::BinCompNotEqual);

		ast[node].child0 = expr0;
		ast[node].child1 = expr1;

		expr_nodes.push(node);
		operators.pop();
	};

	while (parser.has_more() && !parser.next_is(end_token))
	{
		auto& next_token = parser.get();
		if (next_token.type == TokenType::LiteralInteger)
		{
			auto node = ast.make(AstNodeType::LiteralInt);
			ast[node].data_literal_int.value = next_token.data_int;
			expr_nodes.push(node);
		}
		else if (next_token.type == TokenType::LiteralBool)
		{
			auto node = ast.make(AstNodeType::LiteralBool);
			ast[node].data_literal_bool.value = next_token.data_bool;
			expr_nodes.push(node);
		}
		else if (next_token.type == TokenType::OperatorPlus
			  || next_token.type == TokenType::OperatorMultiply
			  || next_token.type == TokenType::CompareGreater
			  || next_token.type == TokenType::CompareGreaterEqual
			  || next_token.type == TokenType::CompareLess
			  || next_token.type == TokenType::CompareLessEqual
			  || next_token.type == TokenType::CompareEqual
			  || next_token.type == TokenType::CompareNotEqual
			  )
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

			if (auto variable_index = scope.find_variable(next_token.data_str))
			{
				auto node = ast.make(AstNodeType::Variable);
				ast[node].data_variable.variable_index = variable_index.value();
				ast[node].data_variable.scope_index = scope_index;
				expr_nodes.push(node);
			}
			else if (auto function = symbol_table.find_function(next_token.data_str))
			{
				auto& function_ref = symbol_table.functions[function.value()];

				auto func_call_node = ast.make(AstNodeType::FunctionCall);
				ast[func_call_node].data_function_call.function_index = function.value();
				expr_nodes.push(func_call_node);

				parser.get_if(TokenType::ParenthesisLeft, "Missing argument list");

				size_t prev_arg_node;
				for (int i = 0; i < function_ref.parameters.size(); i++)
				{
					auto end_token = (i == function_ref.parameters.size() - 1) ? TokenType::ParenthesisRight : TokenType::Comma;
					auto arg_expr_node = parse_expression(parser, ast, symbol_table, scope_index, end_token);
					auto arg_node = ast.make(AstNodeType::FunctionCallArg);
					ast[arg_node].child0 = arg_expr_node;

					if (i == 0)
						ast[func_call_node].child0 = arg_node;
					else
						ast[prev_arg_node].next = arg_node;

					prev_arg_node = arg_node;
				}

				// parse_expression consumes the end token while processing
				// the last parameter
				if (function_ref.parameters.size() == 0)
					parser.get_if(TokenType::ParenthesisRight, "Expected )");
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

	auto& end_of_expr_token = parser.get();
	if (end_of_expr_token.type != end_token)
		internal_error("Expected end token");

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

		auto expr_node = parse_expression(parser, ast, symbol_table, scope_index, TokenType::StatementEnd);
		size_t var_node = ast.make(AstNodeType::Variable);

		auto& scope = symbol_table.scopes[scope_index];
		auto variable_index = scope.find_variable(ident_token.data_str);
		if (!variable_index)
			log_error(ident_token, "Undefined variable");

		ast[var_node].data_variable.variable_index = variable_index.value();
		ast[var_node].data_variable.scope_index = scope_index;

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

		auto expr_node = parse_expression(parser, ast, symbol_table, scope_index, TokenType::StatementEnd);
		size_t var_node = ast.make(AstNodeType::Variable);

		auto& scope = symbol_table.scopes[scope_index];

		auto type_index = symbol_table.find_type(type_token.data_str);
		if (!type_index.has_value())
			log_error(type_token, "Unknown type");

		auto variable_index = scope.make_variable(symbol_table, ident_token.data_str, type_index.value());
		if (!variable_index)
			log_error(ident_token, "Duplicate variable");

		ast[var_node].data_variable.scope_index = scope_index;
		ast[var_node].data_variable.variable_index = variable_index.value();

		size_t assign_node = ast.make(AstNodeType::Assignment);
		ast[assign_node].child0 = var_node;
		ast[assign_node].child1 = expr_node;

		return assign_node;
	}
	// Return
	else if (parser.next_is(TokenType::KeywordReturn))
	{
		auto& return_token = parser.get();

		auto expr_node = parse_expression(parser, ast, symbol_table, scope_index, TokenType::StatementEnd);

		size_t return_node = ast.make(AstNodeType::Return);
		ast[return_node].child0 = expr_node;

		return return_node;
	}
	// Expression
	else
	{
		auto expr_node = parse_expression(parser, ast, symbol_table, scope_index, TokenType::StatementEnd);

		size_t expr_statement_node = ast.make(AstNodeType::ExpressionStatement);
		ast[expr_statement_node].child0 = expr_node;

		return expr_statement_node;
	}
}

void parse_function(Parser& parser, SymbolTable& symbol_table)
{
	parser.get_if(TokenType::KeywordFunctionDecl, "Invalid function declaration");
	auto& func_ident_token = parser.get_if(TokenType::Identifier, "Expected function name");
	parser.get_if(TokenType::ParenthesisLeft, "Expected (");

	if (symbol_table.find_function(func_ident_token.data_str) != std::nullopt)
		log_error(func_ident_token, "Redefined function");

	symbol_table.functions.emplace_back();
	Function& func = symbol_table.functions.back();
	func.name = func_ident_token.data_str;
	func.intrinsic = false;
	size_t func_index = symbol_table.functions.size() - 1;

	size_t func_node = func.ast.make(AstNodeType::FunctionDefinition);
	func.ast_node_root = func_node;

	symbol_table.scopes.emplace_back();
	size_t scope = symbol_table.scopes.size() - 1;

	func.scope = scope;

	while (true)
	{
		if (!parser.has_more())
			log_error("Invalid function declaration");

		if (parser.next_is(TokenType::Identifier, TokenType::Identifier))
		{
			auto& type_token = parser.get();
			auto& ident_token = parser.get();

			// Create variable for the parameter
			auto type_index = symbol_table.find_type(type_token.data_str);
			if (!type_index.has_value())
				log_error(type_token, "Unknown type");

			auto variable_index = symbol_table.scopes[scope].make_variable(symbol_table, ident_token.data_str, type_index.value());
			if (!variable_index)
				log_error(ident_token, "Duplicate parameter name");

			func.parameters.push_back(variable_index.value());

			if (parser.next_is(TokenType::Comma))
				parser.get();
			else if (parser.next_is(TokenType::ParenthesisRight))
			{
				parser.get();
				break;
			}
		}
		else if (parser.next_is(TokenType::ParenthesisRight))
		{
			parser.get();
			break;
		}
		else
		{
			log_error(parser.peek(), "Unexpected token in function declaration");
		}
	}

	parser.get_if(TokenType::Colon, "Missing return type declaration, expected :");
	auto& return_type_token = parser.get_if(TokenType::Identifier, "Expected return type");
	auto return_type_index = symbol_table.find_type(return_type_token.data_str);
	if (!return_type_index.has_value())
		log_error(return_type_token, "Unknown type");
	func.return_type_index = return_type_index.value();

	parser.get_if(TokenType::BraceLeft, "Expected {");

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
		if (stack_size < v.stack_offset)
			stack_size = v.stack_offset;
	}

	if (stack_size % 16 != 0)
		stack_size = ((stack_size / 16) + 1) * 16;

	func.ast[func_node].data_function_definition.stack_size = stack_size;
	func.ast[func_node].data_function_definition.function_index = func_index;
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