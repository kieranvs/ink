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

std::optional<size_t> parse_block(Parser& parser, Ast& ast, SymbolTable& symbol_table, size_t scope, bool create_inner_scope);

bool next_matches_variable(Parser& parser, SymbolTable& symbol_table, size_t scope_index)
{
	auto variable_location = symbol_table.find_variable(scope_index, parser.peek().data_str);
	return variable_location.has_value();
}

size_t parse_variable(Parser& parser, Ast& ast, SymbolTable& symbol_table, size_t init_scope_index)
{
	auto& ident_token = parser.get();
	auto vfr = symbol_table.find_variable(init_scope_index, ident_token.data_str);

	if (!vfr.has_value())
		log_error(ident_token, "Undefined variable");

	auto variable_location = vfr.value();

	auto node = ast.make(variable_location.is_global ? AstNodeType::VariableGlobal :AstNodeType::Variable, ident_token);
	ast[node].data_variable.variable_index = variable_location.variable_index;
	if (!variable_location.is_global) ast[node].data_variable.scope_index = variable_location.scope_index;

	while (parser.next_is(TokenType::Period))
	{
		parser.get(); // period

		auto& parent_variable = variable_location.is_global
			? symbol_table.global_variables[variable_location.variable_index]
			: symbol_table.scopes[variable_location.scope_index].local_variables[variable_location.variable_index];
		auto& parent_variable_type = symbol_table.types[parent_variable.type_index];

		if (parent_variable_type.type != TypeType::Struct)
			log_error(ident_token, "Not a struct");

		auto& ident_token = parser.get_if(TokenType::Identifier, "Expected struct field");

		auto field_location = symbol_table.find_variable(parent_variable_type.scope, ident_token.data_str);

		if (!field_location.has_value())
			log_error(ident_token, "Couldn't find field");

		variable_location = field_location.value();

		auto selector_node = ast.make(AstNodeType::Selector, ident_token);
		ast[selector_node].data_variable.variable_index = variable_location.variable_index;
		ast[selector_node].data_variable.scope_index = variable_location.scope_index;;
		ast[selector_node].child0 = node;

		node = selector_node;
	}

	return node;
}

size_t parse_expression(Parser& parser, Ast& ast, SymbolTable& symbol_table, size_t scope_index, TokenType end_token)
{
	std::stack<size_t> expr_nodes;
	std::stack<std::pair<bool, Token>> operators; // first = is_binary
	size_t index = 0;

	auto priority = [](const std::pair<bool, Token>& token_pair)
	{
		auto& [token_binary, t] = token_pair;

		if (!token_binary)
		{
			if (t.type == TokenType::Asterisk) return 99;
		}

		     if (t.type == TokenType::OperatorPlus)     return 1;
		else if (t.type == TokenType::OperatorMinus)	return 1;
		else if (t.type == TokenType::Asterisk)	        return 2;
		else if (t.type == TokenType::OperatorDivide)	return 2;
		else return 0;
	};

	auto apply_op = [&operators, &expr_nodes, &ast]()
	{
		auto& [top_binary, top] = operators.top();

		size_t node;
		if (top_binary)
		{
			if (expr_nodes.size() < 2)
				log_error(top, "Missing operand for binary operator");

			size_t expr0 = expr_nodes.top();
			expr_nodes.pop();
			size_t expr1 = expr_nodes.top();
			expr_nodes.pop();

			     if (top.type == TokenType::OperatorPlus)        node = ast.make(AstNodeType::BinOpAdd, top);
			else if (top.type == TokenType::OperatorMinus)       node = ast.make(AstNodeType::BinOpSub, top);
			else if (top.type == TokenType::Asterisk)            node = ast.make(AstNodeType::BinOpMul, top);
			else if (top.type == TokenType::OperatorDivide)      node = ast.make(AstNodeType::BinOpDiv, top);
			else if (top.type == TokenType::CompareGreater)      node = ast.make(AstNodeType::BinCompGreater, top);
			else if (top.type == TokenType::CompareGreaterEqual) node = ast.make(AstNodeType::BinCompGreaterEqual, top);
			else if (top.type == TokenType::CompareLess)         node = ast.make(AstNodeType::BinCompLess, top);
			else if (top.type == TokenType::CompareLessEqual)    node = ast.make(AstNodeType::BinCompLessEqual, top);
			else if (top.type == TokenType::CompareEqual)        node = ast.make(AstNodeType::BinCompEqual, top);
			else if (top.type == TokenType::CompareNotEqual)     node = ast.make(AstNodeType::BinCompNotEqual, top);
			else if (top.type == TokenType::LogicalAnd)          node = ast.make(AstNodeType::BinLogicalAnd, top);
			else if (top.type == TokenType::LogicalOr)           node = ast.make(AstNodeType::BinLogicalOr, top);
			else
				internal_error("Invalid binary operator");

			ast[node].child0 = expr0;
			ast[node].child1 = expr1;
		}
		else
		{
			if (!top_binary && expr_nodes.size() < 1)
				log_error(top, "Missing operand for unary operator");

			size_t expr0 = expr_nodes.top();
			expr_nodes.pop();

			if (top.type == TokenType::Asterisk)            node = ast.make(AstNodeType::Dereference, top);
			else
				internal_error("Invalid unary operator");

			ast[node].child0 = expr0;
		}

		expr_nodes.push(node);
		operators.pop();
	};

	bool prev_was_operator_or_nothing = true;
	const Token* prev_token = nullptr;
	while (parser.has_more() && !parser.next_is(end_token))
	{
		prev_token = &parser.peek();
		bool next_is_operator = true;

		if (parser.next_is(TokenType::LiteralInteger))
		{
			auto& next_token = parser.get();
			auto node = ast.make(AstNodeType::LiteralInt, next_token);
			ast[node].data_literal_int.value = next_token.data_int;
			expr_nodes.push(node);
			next_is_operator = false;
		}
		else if (parser.next_is(TokenType::LiteralFloat))
		{
			auto& next_token = parser.get();
			auto node = ast.make(AstNodeType::LiteralFloat, next_token);

			auto float_index = symbol_table.find_add_float(next_token.data_float);

			ast[node].data_literal_float.constant_float_index = float_index;
			expr_nodes.push(node);
			next_is_operator = false;
		}
		else if (parser.next_is(TokenType::LiteralBool))
		{
			auto& next_token = parser.get();
			auto node = ast.make(AstNodeType::LiteralBool, next_token);
			ast[node].data_literal_bool.value = next_token.data_bool;
			expr_nodes.push(node);
			next_is_operator = false;
		}
		else if (parser.next_is(TokenType::LiteralChar))
		{
			auto& next_token = parser.get();
			auto node = ast.make(AstNodeType::LiteralChar, next_token);
			ast[node].data_literal_int.value = next_token.data_int;
			expr_nodes.push(node);
			next_is_operator = false;
		}
		else if (parser.next_is(TokenType::LiteralString))
		{
			auto& next_token = parser.get();
			auto node = ast.make(AstNodeType::LiteralString, next_token);

			auto string_index = symbol_table.find_add_string(next_token.data_str);

			ast[node].data_literal_string.constant_string_index = string_index;
			expr_nodes.push(node);
			next_is_operator = false;
		}
		// Unary operators
		else if (prev_was_operator_or_nothing && parser.next_is(TokenType::Asterisk))
		{
			auto& next_token = parser.get();
			while (!operators.empty() && priority(operators.top()) > priority({ false, next_token })) // or they are the same and next_token is left assoc
			{
				apply_op();
			}

			operators.push({ false, next_token });
		}
		// Binary operators
		else if (!prev_was_operator_or_nothing && (
				 parser.next_is(TokenType::OperatorPlus)
			  || parser.next_is(TokenType::OperatorMinus)
			  || parser.next_is(TokenType::Asterisk)
			  || parser.next_is(TokenType::OperatorDivide)
			  || parser.next_is(TokenType::CompareGreater)
			  || parser.next_is(TokenType::CompareGreaterEqual)
			  || parser.next_is(TokenType::CompareLess)
			  || parser.next_is(TokenType::CompareLessEqual)
			  || parser.next_is(TokenType::CompareEqual)
			  || parser.next_is(TokenType::CompareNotEqual)
			  || parser.next_is(TokenType::LogicalAnd)
			  || parser.next_is(TokenType::LogicalOr)
			  ))
		{
			auto& next_token = parser.get();
			while (!operators.empty() && priority(operators.top()) > priority({ true, next_token })) // or they are the same and next_token is left assoc
			{
				apply_op();
			}

			operators.push({ true, next_token });
		}
		else if (parser.next_is(TokenType::Ampersand))
		{
			auto& next_token = parser.get();

			if (next_matches_variable(parser, symbol_table, scope_index))
			{
				size_t node = parse_variable(parser, ast, symbol_table, scope_index);

				auto address_of_node = ast.make(AstNodeType::AddressOf, next_token);
				ast[address_of_node].child0 = node;

				expr_nodes.push(address_of_node);
			}
			else
				log_error(parser.peek(), "Can't take address of non-variable");

			next_is_operator = false;
		}
		else if (parser.next_is(TokenType::Identifier))
		{
			if (next_matches_variable(parser, symbol_table, scope_index))
			{
				size_t node = parse_variable(parser, ast, symbol_table, scope_index);
				expr_nodes.push(node);
			}
			else if (auto function = symbol_table.find_function(parser.peek().data_str))
			{
				auto& next_token = parser.get();
				auto& function_ref = symbol_table.functions[function.value()];

				// It's a function call
				if (parser.next_is(TokenType::ParenthesisLeft))
				{
					auto func_call_node = ast.make(AstNodeType::FunctionCall, next_token);
					ast[func_call_node].data_function_call.function_index = function.value();
					expr_nodes.push(func_call_node);

					parser.get_if(TokenType::ParenthesisLeft, "Missing argument list");

					size_t prev_arg_node;
					for (int i = 0; i < function_ref.parameters.size(); i++)
					{
						auto end_token = (i == function_ref.parameters.size() - 1) ? TokenType::ParenthesisRight : TokenType::Comma;
						auto& token = parser.peek();
						auto arg_expr_node = parse_expression(parser, ast, symbol_table, scope_index, end_token);
						auto arg_node = ast.make(AstNodeType::FunctionCallArg, token);
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
				// It's a reference to the function itself
				else
				{
					auto func_node = ast.make(AstNodeType::Function, next_token);
					ast[func_node].data_function_call.function_index = function.value();

					auto address_of_node = ast.make(AstNodeType::AddressOf, next_token);
					ast[address_of_node].child0 = func_node;

					expr_nodes.push(address_of_node);
				}
			}
			else
			{
				log_error(parser.peek(), "Undefined variable");
			}

			next_is_operator = false;
		}
		else
		{
			log_error(parser.peek(), "Unexpected token in expression");
		}

		prev_was_operator_or_nothing = next_is_operator;;
	}

	if (!parser.has_more())
	{
		if (prev_token)
			log_error(*prev_token, "Unexpected end of expression");
		else
			log_general_error("Unexpected end of expression");
	}

	auto& end_of_expr_token = parser.get();
	if (end_of_expr_token.type != end_token)
		internal_error("Expected end token");

	while (!operators.empty())
		apply_op();

	if (expr_nodes.empty())
		log_error(end_of_expr_token, "Empty expression");

	return expr_nodes.top();
}

bool next_matches_type(Parser& parser, SymbolTable& symbol_table)
{
	if (parser.next_is(TokenType::Identifier))
	{
		auto& type_token = parser.peek();

		auto type_index = symbol_table.find_type(type_token.data_str);
		return type_index.has_value();
	}

	return false;
}

size_t parse_type(Parser& parser, SymbolTable& symbol_table)
{
	auto& type_token = parser.get();
	auto type_index = symbol_table.find_type(type_token.data_str);
	if (!type_index.has_value())
		log_error(type_token, "Unknown type");

	while (parser.next_is(TokenType::Asterisk))
	{
		auto& asterisk_token = parser.get();
		type_index = get_type_add_pointer(symbol_table, *type_index);
	}

	return type_index.value();
}

size_t parse_statement(Parser& parser, Ast& ast, SymbolTable& symbol_table, size_t scope_index, TokenType end_token = TokenType::StatementEnd)
{
	// Assignment to existing variable
	if (next_matches_variable(parser, symbol_table, scope_index))
	{
		size_t var_node = parse_variable(parser, ast, symbol_table, scope_index);

		auto& assign_token = parser.get();
		auto expr_node = parse_expression(parser, ast, symbol_table, scope_index, end_token);

		size_t assign_node = ast.make(AstNodeType::Assignment, assign_token);
		ast[assign_node].child0 = var_node;
		ast[assign_node].child1 = expr_node;

		return assign_node;
	}
	// Assignment to new variable
	else if (next_matches_type(parser, symbol_table))
	{
		auto type_index = parse_type(parser, symbol_table);

		if (!parser.next_is(TokenType::Identifier))
			log_error(parser.peek(), "Expected identifier");

		auto& ident_token = parser.get();

		// We need to parse the expression before creating the new variable to avoid variable use in defining expression
		size_t expr_node;
		size_t assign_node;
		bool assignment = false;
		if (parser.next_is(TokenType::Assign))
		{
			assignment = true;
			auto& assign_token = parser.get();
			assign_node = ast.make(AstNodeType::Assignment, assign_token);
			expr_node = parse_expression(parser, ast, symbol_table, scope_index, end_token);
		}
		else if (parser.next_is(TokenType::StatementEnd))
		{
			parser.get(); // statement end
		}
		else
			log_error(parser.peek(), "Expected variable declaration");

		size_t var_node = ast.make(AstNodeType::Variable, ident_token);

		auto& scope = symbol_table.scopes[scope_index];

		auto variable_index = scope.make_variable(symbol_table, ident_token.data_str, type_index);
		if (!variable_index)
			log_error(ident_token, "Duplicate variable");

		ast[var_node].data_variable.scope_index = scope_index;
		ast[var_node].data_variable.variable_index = variable_index.value();

		if (assignment)
		{
			ast[assign_node].child0 = var_node;
			ast[assign_node].child1 = expr_node;

			return assign_node;
		}
		else
		{
			size_t init_node = ast.make(AstNodeType::ZeroInitialise, ident_token);
			ast[init_node].child0 = var_node;

			return init_node;
		}
	}
	// Return
	else if (parser.next_is(TokenType::KeywordReturn))
	{
		auto& return_token = parser.get();

		size_t return_node = ast.make(AstNodeType::Return, return_token);
		if (!parser.next_is(TokenType::StatementEnd))
		{
			auto expr_node = parse_expression(parser, ast, symbol_table, scope_index, end_token);
			ast[return_node].aux = expr_node;
		}
		else
		{
			parser.get();
		}

		return return_node;
	}
	// If statement
	else if (parser.next_is(TokenType::KeywordIf))
	{
		auto& if_token = parser.get();
		parser.get_if(TokenType::ParenthesisLeft, "Expected (");

		auto expr_node = parse_expression(parser, ast, symbol_table, scope_index, TokenType::ParenthesisRight);

		auto& brace_token = parser.get_if(TokenType::BraceLeft, "Expected {");

		auto block_node = parse_block(parser, ast, symbol_table, scope_index, true);
		if (!block_node.has_value())
			log_error(brace_token, "Empty body not allowed");

		// Parse else block
		std::optional<size_t> else_block_node;
		if (parser.next_is(TokenType::KeywordElse))
		{
			auto& else_token = parser.get();

			auto& brace_token = parser.get_if(TokenType::BraceLeft, "Expected {");

			else_block_node = parse_block(parser, ast, symbol_table, scope_index, true);
			if (!else_block_node.has_value())
				log_error(brace_token, "Empty body not allowed");
		}

		size_t if_node = ast.make(AstNodeType::If, if_token);
		ast[if_node].child0 = expr_node;
		ast[if_node].child1 = block_node.value();
		ast[if_node].aux = else_block_node;

		return if_node;
	}
	// While loop
	else if (parser.next_is(TokenType::KeywordWhile))
	{
		auto& while_token = parser.get();

		parser.get_if(TokenType::ParenthesisLeft, "Expected (");

		auto expr_node = parse_expression(parser, ast, symbol_table, scope_index, TokenType::ParenthesisRight);

		auto& brace_token = parser.get_if(TokenType::BraceLeft, "Expected {");

		auto block_node = parse_block(parser, ast, symbol_table, scope_index, true);
		if (!block_node.has_value())
			log_error(brace_token, "Empty body not allowed");

		size_t while_node = ast.make(AstNodeType::While, while_token);
		ast[while_node].child0 = expr_node;
		ast[while_node].child1 = block_node.value();

		return while_node;
	}
	// For loop
	else if (parser.next_is(TokenType::KeywordFor))
	{
		auto& for_token = parser.get();

		parser.get_if(TokenType::ParenthesisLeft, "Expected (");

		// Create inner scope manually, rather than in parse_block, because we want
		// to use it for the initialiser statement
		symbol_table.scopes.emplace_back();
		auto inner_scope = symbol_table.scopes.size() - 1;
		symbol_table.scopes[inner_scope].parent = scope_index;

		auto init_node = parse_statement(parser, ast, symbol_table, inner_scope);
		auto cond_node = parse_expression(parser, ast, symbol_table, inner_scope, TokenType::StatementEnd);
		auto incr_node = parse_statement(parser, ast, symbol_table, inner_scope, TokenType::ParenthesisRight);

		auto& brace_token = parser.get_if(TokenType::BraceLeft, "Expected {");

		auto block_node = parse_block(parser, ast, symbol_table, inner_scope, false);
		if (!block_node.has_value())
			log_error(brace_token, "Empty body not allowed");

		size_t for_node = ast.make(AstNodeType::For, for_token);
		ast[for_node].child0 = init_node;
		ast[for_node].child1 = block_node.value();
		ast[init_node].aux = cond_node;
		ast[cond_node].aux = incr_node;

		return for_node;
	}
	// Expression
	else
	{
		auto& token = parser.peek();
		auto expr_node = parse_expression(parser, ast, symbol_table, scope_index, end_token);

		size_t expr_statement_node = ast.make(AstNodeType::ExpressionStatement, token);
		ast[expr_statement_node].child0 = expr_node;

		return expr_statement_node;
	}
}

std::optional<size_t> parse_block(Parser& parser, Ast& ast, SymbolTable& symbol_table, size_t scope, bool create_inner_scope)
{
	if (create_inner_scope)
	{
		auto parent_scope = scope;
		symbol_table.scopes.emplace_back();
		scope = symbol_table.scopes.size() - 1;

		symbol_table.scopes[scope].parent = parent_scope;
	}

	std::optional<size_t> first_node;

	if (parser.next_is(TokenType::BraceRight))
	{
		parser.get();
	}
	else if (parser.has_more())
	{
		first_node = parse_statement(parser, ast, symbol_table, scope);
		size_t prev_node = first_node.value();
		while (parser.has_more())
		{
			if (parser.next_is(TokenType::BraceRight))
			{
				parser.get();
				break;
			}

			size_t st = parse_statement(parser, ast, symbol_table, scope);
			ast[prev_node].next = st;
			prev_node = st;
		}
	}

	return first_node;
}

uint32_t assign_stack_offsets(SymbolTable& symbol_table, uint32_t base_offset, size_t scope_index)
{
	uint32_t scope_size = 0;
	for (auto& v : symbol_table.scopes[scope_index].local_variables)
	{
		v.stack_offset = v.scope_offset + base_offset;

		if (scope_size < v.scope_offset)
			scope_size = v.scope_offset;
	}

	// This is an inefficient way to find the child scopes
	uint32_t biggest_child_size = 0;
	for (size_t i = 0; i < symbol_table.scopes.size(); i++)
	{
		auto& scope = symbol_table.scopes[i];
		if (!scope.parent.has_value() || scope.parent.value() != scope_index) continue;

		uint32_t child_size = assign_stack_offsets(symbol_table, base_offset + scope_size, i);
		if (child_size > biggest_child_size)
			biggest_child_size = child_size;
	}

	return biggest_child_size + scope_size;
}

void parse_function(Parser& parser, SymbolTable& symbol_table, bool is_external)
{
	if (is_external)
		parser.get_if(TokenType::KeywordExternal, "Invalid function declaration");
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

	size_t func_node = func.ast.make(AstNodeType::FunctionDefinition, func_ident_token);
	func.ast_node_root = func_node;

	symbol_table.scopes.emplace_back();
	size_t scope = symbol_table.scopes.size() - 1;

	func.scope = scope;

	// Parse parameter list
	while (true)
	{
		if (!parser.has_more())
			log_error(func_ident_token, "Invalid function declaration");

		if (next_matches_type(parser, symbol_table))
		{
			auto type_index = parse_type(parser, symbol_table);
			auto& ident_token = parser.get();

			// Create variable for the parameter
			auto variable_index = symbol_table.scopes[scope].make_variable(symbol_table, ident_token.data_str, type_index);
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

	if (parser.next_is(TokenType::Colon))
	{
		parser.get();
		func.return_type_index = parse_type(parser, symbol_table);
	}

	if (!is_external)
	{
		parser.get_if(TokenType::BraceLeft, "Expected {");

		func.ast[func_node].next = parse_block(parser, func.ast, symbol_table, scope, false);

		auto stack_size = assign_stack_offsets(symbol_table, 0, scope);
		if (stack_size % 16 != 0)
			stack_size = ((stack_size / 16) + 1) * 16;
		func.ast[func_node].data_function_definition.stack_size = stack_size;
	}
	else
	{
		func.ast[func_node].data_function_definition.stack_size = 0;
		func.is_external = true;
	}

	func.ast[func_node].data_function_definition.function_index = func_index;
}

void parse_function_type(Parser& parser, SymbolTable& symbol_table)
{
	parser.get_if(TokenType::KeywordFunctionType, "Invalid function type declaration");

	auto& ident_token = parser.get_if(TokenType::Identifier, "Expected function type name");
	parser.get_if(TokenType::Assign, "Expected =");
	parser.get_if(TokenType::ParenthesisLeft, "Expected (");

	if (symbol_table.find_type(ident_token.data_str) != std::nullopt)
	log_error(ident_token, "Redefined type");

	std::vector<size_t> parameter_types;
	std::optional<size_t> return_type;

	// Parse parameter list
	while (true)
	{
		if (!parser.has_more())
			log_error(ident_token, "Invalid function declaration");

		if (next_matches_type(parser, symbol_table))
		{
			auto type_index = parse_type(parser, symbol_table);
			parameter_types.push_back(type_index);

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

	if (parser.next_is(TokenType::Colon))
	{
		parser.get();
		return_type = parse_type(parser, symbol_table);
	}

	// Check if there is an existing function type which matches to maintain uniqueness of
	// function types, and make a new one only if necessary
	auto type_index = symbol_table.find_matching_function_type(parameter_types, return_type);
	if (!type_index.has_value())
	{
		symbol_table.types.emplace_back();
		auto& type = symbol_table.types.back();
		type.type = TypeType::Function;
		type.data_size = 8;
		type.function_type_index = symbol_table.function_types.size();

		symbol_table.function_types.emplace_back();
		FunctionType& function_type = symbol_table.function_types.back();
		function_type.parameter_types = std::move(parameter_types);
		function_type.return_type_index = return_type;

		type_index = symbol_table.types.size() - 1;
	}

	// Make an alias to the actual function type
	symbol_table.types.emplace_back();
	auto& type = symbol_table.types.back();
	type.type = TypeType::Alias;
	type.name = ident_token.data_str;
	type.actual_type = type_index.value();
}

void parse_struct(Parser& parser, SymbolTable& symbol_table)
{
	parser.get_if(TokenType::KeywordStruct, "Invalid struct declaration");

	auto& struct_ident_token = parser.get_if(TokenType::Identifier, "Expected struct name");
	parser.get_if(TokenType::BraceLeft, "Expected {");

	if (symbol_table.find_type(struct_ident_token.data_str) != std::nullopt)
		log_error(struct_ident_token, "Redefined type");

	symbol_table.types.emplace_back();
	auto type_index = symbol_table.types.size() - 1;
	symbol_table.scopes.emplace_back();
	auto scope_index = symbol_table.scopes.size() - 1;

	{
		auto& struct_type = symbol_table.types.back();
		struct_type.name = struct_ident_token.data_str;
		struct_type.type = TypeType::Struct;
		struct_type.scope = scope_index;
	}

	while (true)
	{
		if (!parser.has_more())
			log_error(struct_ident_token, "Invalid struct declaration");

		if (next_matches_type(parser, symbol_table))
		{
			auto field_type_index = parse_type(parser, symbol_table);
			auto& ident_token = parser.get_if(TokenType::Identifier, "Expected identifier");

			parser.get_if(TokenType::StatementEnd, "Expected ;");

			auto variable_index = symbol_table.scopes[scope_index].make_variable(symbol_table, ident_token.data_str, field_type_index);
			if (!variable_index)
				log_error(ident_token, "Duplicate struct field");
		}
		else if (parser.next_is(TokenType::BraceRight))
		{
			parser.get();
			break;
		}
		else
		{
			log_error(parser.peek(), "Unexpected token in struct declaration");
		}
	}

	symbol_table.types[type_index].data_size = assign_stack_offsets(symbol_table, 0, scope_index);
}

void parse_top_level(Parser& parser, SymbolTable& symbol_table)
{
	while (parser.has_more())
	{
		if (parser.next_is(TokenType::KeywordFunctionDecl))
			parse_function(parser, symbol_table, false);
		else if (parser.next_is(TokenType::KeywordStruct))
			parse_struct(parser, symbol_table);
		else if (parser.next_is(TokenType::KeywordExternal))
			parse_function(parser, symbol_table, true);
		else if (parser.next_is(TokenType::KeywordFunctionType))
			parse_function_type(parser, symbol_table);
		else if (parser.next_is(TokenType::DirectiveLink) || parser.next_is(TokenType::DirectiveLinkFramework))
		{
			auto& link_token = parser.get();
			auto& path_token = parser.get_if(TokenType::LiteralString, "Expected linker path");

			symbol_table.add_linker_path(path_token.data_str, link_token.type == TokenType::DirectiveLinkFramework);
		}
		else if (next_matches_type(parser, symbol_table))
		{
			auto type_index = parse_type(parser, symbol_table);

			auto& ident_token = parser.get_if(TokenType::Identifier, "Expected identifier");
			parser.get_if(TokenType::StatementEnd, "Expected ;");

			auto& var = symbol_table.global_variables.emplace_back();
			var.name = ident_token.data_str;
			var.type_index = type_index;
		}
		else
			log_error(parser.peek(), "Unexpected token at top level");
	}
}