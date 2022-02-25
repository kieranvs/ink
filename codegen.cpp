#include "codegen.h"

#include "utils.h"

void codegen(Ast& ast, FILE* file, size_t index)
{
	if (ast[index].type == AstNodeType::None)
		return;
	else if (ast[index].type == AstNodeType::LiteralInt)
		fprintf(file, "    mov rax, %d\n", ast[index].data_literal_int.value);
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
		fprintf(file, "    mov eax, [rbp - %d]\n", ast[index].data_variable.offset);
	}
	else if (ast[index].type == AstNodeType::Assignment)
	{
		codegen(ast, file, ast[index].child1);

		auto variable_node = ast[index].child0;
		fprintf(file, "    mov [rbp - %d], eax\n", ast[variable_node].data_variable.offset);

		if (ast[index].next.has_value())
			codegen(ast, file, ast[index].next.value());
	}
	else if (ast[index].type == AstNodeType::FunctionDefinition)
	{
		fprintf(file, "    push rbp\n");
		fprintf(file, "    mov rbp, rsp\n");
		fprintf(file, "    sub rsp, %d\n", ast[index].data_function_definition.stack_size);

		if (ast[index].next.has_value())
			codegen(ast, file, ast[index].next.value());

		fprintf(file, "    leave\n");
	}
	else
	{
		fail("Unhandled AST node type in code gen\n");
	}
}
