#include "ast.h"

#include "utils.h"

#include <stdio.h>

void dump_ast(Ast& ast, size_t index, int indent)
{
	for (int i = 0; i < indent; i++)
		printf("  ");

	if (ast[index].type == AstNodeType::None)
		return;
	else if (ast[index].type == AstNodeType::LiteralInt)
		printf("%d\n", ast[index].data_int);
	else if (ast[index].type == AstNodeType::BinOpAdd)
	{
		printf("+\n");
		dump_ast(ast, ast[index].child0, indent + 1);
		dump_ast(ast, ast[index].child1, indent + 1);
	}
	else if (ast[index].type == AstNodeType::BinOpMul)
	{
		printf("*\n");
		dump_ast(ast, ast[index].child0, indent + 1);
		dump_ast(ast, ast[index].child1, indent + 1);
	}
	else if (ast[index].type == AstNodeType::Assignment)
	{
		printf("=\n");
		dump_ast(ast, ast[index].child0, indent + 1);
		dump_ast(ast, ast[index].child1, indent + 1);
		if (ast[index].next.has_value())
			dump_ast(ast, ast[index].next.value(), indent);
	}
	else if (ast[index].type == AstNodeType::Variable)
	{
		printf("Variable %d\n", ast[index].data_int);
	}
	else if (ast[index].type == AstNodeType::FunctionDefinition)
	{
		printf("Function\n");
		if (ast[index].next.has_value())
			dump_ast(ast, ast[index].next.value(), indent + 1);
	}
	else
	{
		fail("Unhandled AST node type in dump_ast\n");
	}
}

const Variable& Scope::find_or_make_variable(const std::string& name)
{
	for (const auto& v : local_variables)
	{
		if (v.name == name) return v;
	}

	local_variables.emplace_back();
	auto& v = local_variables.back();

	v.name = name;
	if (local_variables.size() == 1)
		v.stack_offset = 0;
	else
		v.stack_offset = local_variables[local_variables.size() - 2].stack_offset + 4;

	return v;
}