#include "codegen.h"

#include "utils.h"

const char* register_for_parameter(int i, int b)
{
	     if (i == 0) return (b == 64) ? "rdi" : "edi";
	else if (i == 1) return (b == 64) ? "rsi" : "esi";
	else if (i == 2) return (b == 64) ? "rdx" : "edx";
	else if (i == 3) return (b == 64) ? "rcx" : "ecx";
	else if (i == 4) return (b == 64) ? "r8"  : "r8d";
	else if (i == 5) return (b == 64) ? "r9"  : "r9d";
	else
		internal_error("Register overflow");
}

void codegen_ast(Ast& ast, SymbolTable& symbol_table, FILE* file, size_t index)
{
	if (ast[index].type == AstNodeType::None)
		return;
	else if (ast[index].type == AstNodeType::LiteralInt)
		fprintf(file, "    mov rax, %d\n", ast[index].data_literal_int.value);
	else if (ast[index].type == AstNodeType::BinOpAdd)
	{
		codegen_ast(ast, symbol_table, file, ast[index].child0);
		fprintf(file, "    push rax\n");
		codegen_ast(ast, symbol_table, file, ast[index].child1);
		fprintf(file, "    pop rbx\n");
		fprintf(file, "    add rax, rbx\n");
	}
	else if (ast[index].type == AstNodeType::BinOpMul)
	{
		codegen_ast(ast, symbol_table, file, ast[index].child0);
		fprintf(file, "    push rax\n");
		codegen_ast(ast, symbol_table, file, ast[index].child1);
		fprintf(file, "    pop rbx\n");
		fprintf(file, "    mul rbx\n");
	}
	else if (ast[index].type == AstNodeType::Variable)
	{
		fprintf(file, "    mov eax, [rbp - %d]\n", ast[index].data_variable.offset);
	}
	else if (ast[index].type == AstNodeType::Assignment)
	{
		codegen_ast(ast, symbol_table, file, ast[index].child1);

		auto variable_node = ast[index].child0;
		fprintf(file, "    mov [rbp - %d], eax\n", ast[variable_node].data_variable.offset);

		if (ast[index].next.has_value())
			codegen_ast(ast, symbol_table, file, ast[index].next.value());
	}
	else if (ast[index].type == AstNodeType::FunctionDefinition)
	{
		fprintf(file, "    push rbp\n");
		fprintf(file, "    mov rbp, rsp\n");
		fprintf(file, "    sub rsp, %d\n", ast[index].data_function_definition.stack_size);

		auto func_index = ast[index].data_function_definition.function_index;
		auto& func = symbol_table.functions[func_index];
		auto& func_scope = symbol_table.scopes[func.scope];

		for (int i = 0; i < func.parameters.size(); i++)
		{
			auto param_offset = func_scope.local_variables[func.parameters[i]].stack_offset;
			fprintf(file, "    mov [rbp - %d], %s\n", param_offset, register_for_parameter(i, 32));
		}

		if (ast[index].next.has_value())
			codegen_ast(ast, symbol_table, file, ast[index].next.value());

		fprintf(file, "    leave\n");
		fprintf(file, "    ret\n");
	}
	else if (ast[index].type == AstNodeType::FunctionCall)
	{
		auto func_index = ast[index].data_function_call.function_index;
		auto& func = symbol_table.functions[func_index];
		auto& func_scope = symbol_table.scopes[func.scope];

		if (func.parameters.size() != 0)
		{
			auto current_arg_node = ast[index].child0;

			for (int i = 0; i < func.parameters.size(); i++)
			{
				codegen_ast(ast, symbol_table, file, ast[current_arg_node].child0);
				fprintf(file, "    mov %s, rax\n", register_for_parameter(i, 64));

				current_arg_node = ast[current_arg_node].next.value_or(current_arg_node);
			}
		}

		fprintf(file, "    call %s\n", func.name.c_str());
	}
	else if (ast[index].type == AstNodeType::Return)
	{
		codegen_ast(ast, symbol_table, file, ast[index].child0);
	}
	else
	{
		internal_error("Unhandled AST node type in code gen");
	}
}

void codegen_function(Function& func, SymbolTable& symbol_table, FILE* file)
{
	fprintf(file, "%s:\n", func.name.c_str());
	codegen_ast(func.ast, symbol_table, file, func.ast_node_root);
	fprintf(file, "\n");
}

void codegen(SymbolTable& symbol_table, FILE* file)
{
	// Check that the main function is defined
	bool main_defined = false;
	for (auto& func : symbol_table.functions)
	{
		if (func.name == "main")
		{
			main_defined = true;
			break;
		}
	}
	if (!main_defined) log_error("No main function defined");

	fprintf(file, "    global    _start\n");
	fprintf(file, "\n");
	fprintf(file, "    section   .text\n");
	fprintf(file, "\n");
	fprintf(file, "_start:\n");
	fprintf(file, "    call main\n");
	fprintf(file, "    mov rdi, rax\n");
	fprintf(file, "    call print_uint32\n");
	fprintf(file, "    call exit\n");
	fprintf(file, "\n");

	fprintf(file, "; user code\n");
	for (auto& func : symbol_table.functions)
	{
		codegen_function(func, symbol_table, file);
	}

	fprintf(file, "; intrinsics\n");
	fprintf(file, "exit:\n");
	fprintf(file, "    mov rax, 60\n");
	fprintf(file, "    xor rdi, rdi\n");
	fprintf(file, "    syscall\n");
	fprintf(file, "\n");
	fprintf(file, "print_uint32:\n");
	fprintf(file, "    mov eax, edi\n");
	fprintf(file, "    mov ecx, 10\n");
	fprintf(file, "    push rcx\n");
	fprintf(file, "    mov rsi, rsp\n");
	fprintf(file, "    sub rsp, 16\n");
	fprintf(file, ".toascii_digit:\n");
	fprintf(file, "    xor edx, edx\n");
	fprintf(file, "    div ecx\n");
	fprintf(file, "    add edx, '0'\n");
	fprintf(file, "    dec rsi\n");
	fprintf(file, "    mov [rsi], dl\n");
	fprintf(file, "    test eax, eax\n");
	fprintf(file, "    jnz .toascii_digit\n");
	fprintf(file, "    mov eax, 1\n");
	fprintf(file, "    mov edi, 1\n");
	fprintf(file, "    lea edx, [rsp+16 + 1]\n");
	fprintf(file, "    sub edx, esi\n");
	fprintf(file, "    syscall\n");
	fprintf(file, "    add rsp, 24\n");
	fprintf(file, "    ret\n");
}
