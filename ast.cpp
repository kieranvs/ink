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
	}
	else if (ast[index].type == AstNodeType::Variable)
	{
		printf("Variable\n");
	}
	else
	{
		fail("Unhandled AST node type in dump_ast\n");
	}
}