#include "codegen.h"

#include "utils.h"

const char* register_name_data[] =
{
	 "al", "ax", "eax", "rax",
	 "bl", "bx", "ebx", "rbx",
	 "cl", "cx", "ecx", "rcx",
	 "dl", "dx", "edx", "rdx",
	"sil", "si", "esi", "rsi",
	"dil", "di", "edi", "rdi",
	"bpl", "bp", "ebp", "rbp",
	"spl", "sp", "esp", "rsp",
	"r8b", "r8w", "r8d", "r8",
	"r9b", "r9w", "r9d", "r9",
	"r10b", "r10w", "r10d", "r10",
	"r11b", "r11w", "r11d", "r11",
	"r12b", "r12w", "r12d", "r12",
	"r13b", "r13w", "r13d", "r13",
	"r14b", "r14w", "r14d", "r14",
	"r15b", "r15w", "r15d", "r15"
};

const char* register_name(int reg, int bytes)
{
	     if (bytes == 1) return register_name_data[reg * 4 + 0];
	else if (bytes == 2) return register_name_data[reg * 4 + 1];
	else if (bytes == 4) return register_name_data[reg * 4 + 2];
	else if (bytes == 8) return register_name_data[reg * 4 + 3];
	else
		internal_error("Invalid register name request");
}

int register_for_parameter(int i)
{
         if (i == 0) return 5;
	else if (i == 1) return 4;
	else if (i == 2) return 3;
	else if (i == 3) return 2;
	else if (i == 4) return 8;
	else if (i == 5) return 9;
	else
		internal_error("Register overflow");
}

void codegen_ast(Ast& ast, SymbolTable& symbol_table, FILE* file, size_t index)
{
	if (ast[index].type == AstNodeType::None)
		return;
	else if (ast[index].type == AstNodeType::LiteralInt)
		fprintf(file, "    mov %s, %d\n", register_name(0, 8), ast[index].data_literal_int.value);
	else if (ast[index].type == AstNodeType::LiteralBool)
	{
		if (ast[index].data_literal_bool.value)
			fprintf(file, "    mov %s, %d\n", register_name(0, 1), 1);
		else
			fprintf(file, "    mov %s, %d\n", register_name(0, 1), 0);
	}
	else if (ast[index].type == AstNodeType::BinOpAdd)
	{
		codegen_ast(ast, symbol_table, file, ast[index].child0);
		fprintf(file, "    push %s\n", register_name(0, 8));
		codegen_ast(ast, symbol_table, file, ast[index].child1);
		fprintf(file, "    pop %s\n", register_name(2, 8));
		fprintf(file, "    add %s, %s\n", register_name(0, 8), register_name(2, 8));
	}
	else if (ast[index].type == AstNodeType::BinOpMul)
	{
		codegen_ast(ast, symbol_table, file, ast[index].child0);
		fprintf(file, "    push %s\n", register_name(0, 8));
		codegen_ast(ast, symbol_table, file, ast[index].child1);
		fprintf(file, "    pop %s\n", register_name(2, 8));
		fprintf(file, "    mul %s\n", register_name(2, 8));
	}
	else if (ast[index].type == AstNodeType::Variable)
	{
		auto& scope = symbol_table.scopes[ast[index].data_variable.scope_index];
		auto& variable = scope.local_variables[ast[index].data_variable.variable_index];
		auto& type = symbol_table.types[variable.type_index];
		auto data_size = type.data_size;
		fprintf(file, "    mov %s, [rbp - %d]\n", register_name(0, data_size), variable.stack_offset);
	}
	else if (ast[index].type == AstNodeType::Assignment)
	{
		codegen_ast(ast, symbol_table, file, ast[index].child1);

		auto variable_node = ast[index].child0;
		auto& scope = symbol_table.scopes[ast[variable_node].data_variable.scope_index];
		auto& variable = scope.local_variables[ast[variable_node].data_variable.variable_index];
		auto& type = symbol_table.types[variable.type_index];
		auto data_size = type.data_size;
		fprintf(file, "    mov [rbp - %d], %s\n", variable.stack_offset, register_name(0, data_size));

		if (ast[index].next.has_value())
			codegen_ast(ast, symbol_table, file, ast[index].next.value());
	}
	else if (ast[index].type == AstNodeType::ExpressionStatement)
	{
		codegen_ast(ast, symbol_table, file, ast[index].child0);

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
			auto var_type = func_scope.local_variables[func.parameters[i]].type_index;
			auto data_size = symbol_table.types[var_type].data_size;
			fprintf(file, "    mov [rbp - %d], %s\n", param_offset, register_name(register_for_parameter(i), data_size));
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
				fprintf(file, "    mov %s, %s\n", register_name(register_for_parameter(i), 8), register_name(0, 8));

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
			if (symbol_table.types[func.return_type_index].name != "int")
				log_error("Main function defined with wrong return type");

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
		if (!func.intrinsic)
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
