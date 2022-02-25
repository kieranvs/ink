#include "parser.h"

#include <stack>

size_t parse_expression(Parser& parser, Ast& ast)
{
	std::stack<size_t> expr_nodes;
	std::stack<Token> operators;
	size_t index = 0;

	auto priority = [](const Token& t)
	{
		if (t.type == TokenType::Plus)
			return 1;
		else if (t.type == TokenType::Multiply)
			return 2;
		else return 0;
	};

	auto apply_op = [&operators, &expr_nodes, &ast]()
	{
		auto& top = operators.top();
		
		size_t expr0 = expr_nodes.top();
		expr_nodes.pop();
		size_t expr1 = expr_nodes.top();
		expr_nodes.pop();

		size_t node;
		if (top.type == TokenType::Plus)
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
		if (next_token.type == TokenType::IntegerLiteral)
		{
			auto node = ast.make(AstNodeType::LiteralInt);
			ast[node].data_int = next_token.data_int;
			expr_nodes.push(node);
		}
		else if (next_token.type == TokenType::Plus || next_token.type == TokenType::Multiply)
		{
			while (!operators.empty() && priority(operators.top()) > priority(next_token)) // or they are the same and next_token is left assoc
			{
				apply_op();
			}

			operators.push(next_token);
		}
		else
		{
			fail("Unexpected token in expression\n");
		}
	}

	while (parser.next_is(TokenType::StatementEnd)) parser.get();

	while (!operators.empty())
		apply_op();

	return expr_nodes.top();
}

size_t parse_statement(Parser& parser, Ast& ast, SymbolTable& symbol_table, size_t scope_index)
{
	auto& ident_token = parser.get();
	if (ident_token.type == TokenType::Identifier)
	{
		auto& assign_token = parser.get();
		if (assign_token.type != TokenType::Assign) fail("Invalid statement\n");

		auto expr_node = parse_expression(parser, ast);
		size_t var_node = ast.make(AstNodeType::Variable);

		auto& scope = symbol_table.scopes[scope_index];
		auto& variable = scope.find_or_make_variable(ident_token.data_str);
		ast[var_node].data_int = variable.stack_offset;

		size_t assign_node = ast.make(AstNodeType::Assignment);
		ast[assign_node].child0 = var_node;
		ast[assign_node].child1 = expr_node;

		return assign_node;
	}
	else
	{
		fail("Invalid statement\n");
		return 0;
	}
}

void parse_function(Parser& parser, SymbolTable& symbol_table, size_t func_index)
{
	Ast& ast = symbol_table.functions[func_index].ast;
	size_t func_node = ast.make(AstNodeType::FunctionDefinition);
	symbol_table.functions[func_index].ast_node_root = func_node;

	symbol_table.scopes.emplace_back();
	size_t scope = symbol_table.scopes.size() - 1;

	if (parser.has_more())
	{
		ast[func_node].next = parse_statement(parser, ast, symbol_table, scope);
		size_t prev_node = ast[func_node].next.value();
		while (parser.has_more())
		{
			size_t st = parse_statement(parser, ast, symbol_table, scope);
			ast[prev_node].next = st;
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

	ast[func_node].data_int = stack_size;
}