#include "codegen.h"

#include "utils.h"

void codegen(Ast& ast, FILE* file, size_t index)
{
	if (ast[index].type == AstNodeType::None)
		return;
	else if (ast[index].type == AstNodeType::LiteralInt)
		fprintf(file, "    mov rax, %d\n", ast[index].data_int);
	else if (ast[index].type == AstNodeType::BinOpAdd)
	{
		codegen(ast, file, ast[index].child0);
		fprintf(file, "    push rax\n");
		codegen(ast, file, ast[index].child1);
		fprintf(file, "    pop rbx\n");
		fprintf(file, "    add rax, rbx\n");
	}
	else if (ast[index].type == AstNodeType::BinOpMul)
	{
		codegen(ast, file, ast[index].child0);
		fprintf(file, "    push rax\n");
		codegen(ast, file, ast[index].child1);
		fprintf(file, "    pop rbx\n");
		fprintf(file, "    mul rbx\n");
	}
	else if (ast[index].type == AstNodeType::Variable)
	{
		
	}
	else if (ast[index].type == AstNodeType::Assignment)
	{
		codegen(ast, file, ast[index].child1);
	}
	else
	{
		fail("Unhandled AST node type in code gen\n");
	}
}
