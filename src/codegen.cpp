#include "codegen.h"
#include "typecheck.h"

#include "utils.h"

const char* register_name_data[] =
{
	 "al", "ax", "eax", "rax",     // 0
	 "bl", "bx", "ebx", "rbx",     // 1
	 "cl", "cx", "ecx", "rcx",     // 2
	 "dl", "dx", "edx", "rdx",     // 3
	"sil", "si", "esi", "rsi",     // 4
	"dil", "di", "edi", "rdi",     // 5
	"bpl", "bp", "ebp", "rbp",     // 6
	"spl", "sp", "esp", "rsp",     // 7
	"r8b", "r8w", "r8d", "r8",     // 8
	"r9b", "r9w", "r9d", "r9",     // 9
	"r10b", "r10w", "r10d", "r10", // 10
	"r11b", "r11w", "r11d", "r11", // 11
	"r12b", "r12w", "r12d", "r12", // 12
	"r13b", "r13w", "r13d", "r13", // 13
	"r14b", "r14w", "r14d", "r14", // 14
	"r15b", "r15w", "r15d", "r15"  // 15
};

const char* register_name(int reg, int bytes)
{
	if (reg < 0 || reg >= 16) internal_error("Invalid register");

	     if (bytes == 1) return register_name_data[reg * 4 + 0];
	else if (bytes == 2) return register_name_data[reg * 4 + 1];
	else if (bytes == 4) return register_name_data[reg * 4 + 2];
	else if (bytes == 8) return register_name_data[reg * 4 + 3];
	else
		internal_error("Invalid register size");
}

const char* xmm_register_name_data[] =
{
	"xmm0", "xmm1", "xmm2", "xmm3",
	"xmm4", "xmm5", "xmm6", "xmm7",
	"xmm8", "xmm9", "xmm10", "xmm11",
	"xmm12", "xmm13", "xmm14", "xmm15"
};

const char* xmm_register_name(int reg)
{
	if (reg < 16 || reg >= 32) internal_error("Invalid xmm register");

	return xmm_register_name_data[reg - 16];
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

int xmm_register_for_parameter(int i)
{
	if (i < 0 || i >= 8) internal_error("Float parameter register overflow");

	return 16 + i;
}

// "Volatile"/"Call clobbered registers" are free to use within a function but need to be saved before a call
int caller_saved_registers[] = { 0, 2, 3, 4, 5, 8, 9, 10, 11 };
// "Call preserved registers" need to be saved and restored within the function if they are used
int callee_saved_registers[] = { 1, 12, 13, 14, 15 };

uint8_t RegisterStatusFlag_InUse            = 1;
uint8_t RegisterStatusFlag_ContainsVariable = 2;

struct RegisterStatus
{
	uint8_t flags = 0;

	// Set if the register stores a variable
	uint32_t stack_offset;
	uint8_t stack_size;

	bool has_flag(uint8_t flag)
	{
		return (flags & flag) != 0;
	}

	void set_all_flags(uint8_t new_flags)
	{
		flags = new_flags;
	}

	void set_flag(uint8_t flag)
	{
		flags |= flag;
	}

	void unset_flag(uint8_t flag)
	{
		flags &= (~flag);
	}
};

struct RegisterState
{
	RegisterState()
	{
		for (int i = 0; i < 32; i++)
			register_status[i].set_all_flags(0);
	}

	RegisterStatus register_status[32];

	int get_free_register(uint8_t flags)
	{
		// For now we only use the caller saved registers
		for (int i = 0; i < 9; i++)
		{
			int r = caller_saved_registers[i];
			if (!register_status[r].has_flag(RegisterStatusFlag_InUse))
			{
				register_status[r].set_all_flags(flags);
				return r;
			}
		}

		internal_error("No free registers - expression too complex");
		return 0;
	}

	int get_free_xmm_register(uint8_t flags)
	{
		for (int i = 16; i < 32; i++)
		{
			if (!register_status[i].has_flag(RegisterStatusFlag_InUse))
			{
				register_status[i].set_all_flags(flags);
				return i;
			}
		}

		internal_error("No free xmm registers");
		return 0;
	}

	void dump()
	{
		for (int i = 0; i < 32; i++)
		{
			auto& rs = register_status[i];

			if (i < 16)
				printf("  %s : ", register_name(i, 8));
			else
				printf("  %s : ", xmm_register_name(i));

			if (rs.has_flag(RegisterStatusFlag_InUse)) printf("in_use ");
			if (rs.has_flag(RegisterStatusFlag_ContainsVariable)) printf("contains_variable ");
			if (rs.has_flag(RegisterStatusFlag_ContainsVariable))
			{
				printf("stack_offset=%d stack_size=%d ", rs.stack_offset, rs.stack_size);
			}
			printf("\n");
		}
	}
};

// Returns (stack_offset, data_size) for the data referred to by the given
// variable or selector ast node.
// stack_offset is the number of bytes below the stack pointer where the
// first (low) byte of the struct sits.
std::pair<uint32_t, size_t> compute_stack_offset_and_size(Ast& ast, SymbolTable& symbol_table, size_t node_index)
{
	auto& ast_node = ast[node_index];
	if (ast_node.type != AstNodeType::Variable && ast_node.type != AstNodeType::Selector)
		internal_error("compute_stack_offset_and_size invalid ast node");

	auto& scope = symbol_table.scopes[ast_node.data_variable.scope_index];
	auto& variable = scope.local_variables[ast_node.data_variable.variable_index];
	auto& type = symbol_table.types[variable.type_index];
	auto data_size = type.data_size;

	if (ast_node.type == AstNodeType::Selector)
	{
		// Subtract the selector's offset from the parent, because
		// we want to climb upwards in the stack - which is subtraction
		// to this stack_offset. Add data_size to get to the low byte of
		// the selector variable. Struct scopes are inverted compared
		// to normal scopes which causes this confusion.
		uint32_t stack_offset = compute_stack_offset_and_size(ast, symbol_table, ast_node.child0).first - variable.stack_offset + data_size;
		return { stack_offset, data_size };
	}
	else
	{
		return { variable.stack_offset, data_size };
	}
}

void codegen_binop(Ast& ast, size_t index, int r0, int r1, size_t arg_size, FILE* file, RegisterState& registers)
{
	if (ast[index].type == AstNodeType::BinOpAdd)
		fprintf(file, "    add %s, %s\n", register_name(r0, arg_size), register_name(r1, arg_size));
	else if (ast[index].type == AstNodeType::BinOpSub)
		fprintf(file, "    sub %s, %s\n", register_name(r0, arg_size), register_name(r1, arg_size));
	else if (ast[index].type == AstNodeType::BinOpMul)
		fprintf(file, "    imul %s, %s\n", register_name(r0, arg_size), register_name(r1, arg_size));
	else if (ast[index].type == AstNodeType::BinOpDiv)
	{
		// rdx needs to be 0
		// result = expr_0 / expr_1
		// expr_0 needs to be in rax

		if (r1 == 0)
		{
			fprintf(file, "    push %s\n", register_name(r1, 8));
			fprintf(file, "    mov %s, %s\n", register_name(r1, arg_size), register_name(r0, arg_size));
			fprintf(file, "    pop %s\n", register_name(r0, 8));
			std::swap(r0, r1);
		}

		bool pop_rax = false;
		bool pop_rdx = false;

		if (r0 != 0 && registers.register_status[0].has_flag(RegisterStatusFlag_InUse))
		{
			fprintf(file, "    push %s\n", register_name(0, 8));
			pop_rax = true;
		}

		if (registers.register_status[3].has_flag(RegisterStatusFlag_InUse))
		{
			fprintf(file, "    push %s\n", register_name(3, 8));
			pop_rdx = true;
		}

		// Set rdx to 0
		fprintf(file, "    mov %s, %d\n", register_name(3, arg_size), 0);

		// Move expr_0 into rax
		if (r0 != 0) fprintf(file, "    mov %s, %s\n", register_name(0, arg_size), register_name(r0, arg_size));

		// Do the divide, result is in rax
		fprintf(file, "    div %s\n", register_name(r1, arg_size));

		// Move result to r0
		if (r0 != 0) fprintf(file, "    mov %s, %s\n", register_name(r0, arg_size), register_name(0, arg_size));

		if (pop_rdx)
			fprintf(file, "    pop %s\n", register_name(3, 8));

		if (pop_rax)
			fprintf(file, "    pop %s\n", register_name(0, 8));
	}
	else if (ast[index].type == AstNodeType::BinLogicalAnd)
		fprintf(file, "    and %s, %s\n", register_name(r0, 1), register_name(r1, 1));
	else if (ast[index].type == AstNodeType::BinLogicalOr)
		fprintf(file, "    or %s, %s\n", register_name(r0, 1), register_name(r1, 1));
	else
	{
		fprintf(file, "    cmp %s, %s\n", register_name(r0, arg_size), register_name(r1, arg_size));
		if (ast[index].type == AstNodeType::BinCompGreater)
			fprintf(file, "    setg %s\n", register_name(r0, 1));
		else if (ast[index].type == AstNodeType::BinCompGreaterEqual)
			fprintf(file, "    setge %s\n", register_name(r0, 1));
		else if (ast[index].type == AstNodeType::BinCompLess)
			fprintf(file, "    setl %s\n", register_name(r0, 1));
		else if (ast[index].type == AstNodeType::BinCompLessEqual)
			fprintf(file, "    setle %s\n", register_name(r0, 1));
		else if (ast[index].type == AstNodeType::BinCompEqual)
			fprintf(file, "    sete %s\n", register_name(r0, 1));
		else if (ast[index].type == AstNodeType::BinCompNotEqual)
			fprintf(file, "    setne %s\n", register_name(r0, 1));
		else
			internal_error("Unhandled binary compare");
	}
}

int codegen_binop_float(Ast& ast, size_t index, int r0, int r1, FILE* file, RegisterState& registers)
{
	if (ast[index].type == AstNodeType::BinOpAdd)
		fprintf(file, "    addsd %s, %s\n", xmm_register_name(r0), xmm_register_name(r1));
	else if (ast[index].type == AstNodeType::BinOpSub)
		fprintf(file, "    subsd %s, %s\n", xmm_register_name(r0), xmm_register_name(r1));
	else if (ast[index].type == AstNodeType::BinOpMul)
		fprintf(file, "    mulsd %s, %s\n", xmm_register_name(r0), xmm_register_name(r1));
	else if (ast[index].type == AstNodeType::BinOpDiv)
		fprintf(file, "    divsd %s, %s\n", xmm_register_name(r0), xmm_register_name(r1));
	else
	{
		int r2 = registers.get_free_register(RegisterStatusFlag_InUse);

		if (ast[index].type == AstNodeType::BinCompLess
		 || ast[index].type == AstNodeType::BinCompLessEqual)
		{
			fprintf(file, "    comisd %s, %s\n", xmm_register_name(r1), xmm_register_name(r0));
			if (ast[index].type == AstNodeType::BinCompLess)
				fprintf(file, "    seta %s\n", register_name(r2, 1));
			else if (ast[index].type == AstNodeType::BinCompLessEqual)
				fprintf(file, "    setnb %s\n", register_name(r2, 1));
		}
		else
		{
			fprintf(file, "    comisd %s, %s\n", xmm_register_name(r0), xmm_register_name(r1));
			if (ast[index].type == AstNodeType::BinCompGreater)
				fprintf(file, "    seta %s\n", register_name(r2, 1));
			else if (ast[index].type == AstNodeType::BinCompGreaterEqual)
				fprintf(file, "    setnb %s\n", register_name(r2, 1));
			else if (ast[index].type == AstNodeType::BinCompEqual)
				fprintf(file, "    sete %s\n", register_name(r2, 1));
			else if (ast[index].type == AstNodeType::BinCompNotEqual)
				fprintf(file, "    setne %s\n", register_name(r2, 1));
			else
				internal_error("Unhandled float binary operation");
		}

		return r2;
	}

	return r0;
}

int codegen_expr(Ast& ast, SymbolTable& symbol_table, FILE* file, size_t index, RegisterState& registers)
{
	if (ast[index].type == AstNodeType::LiteralInt)
	{
		int r = registers.get_free_register(RegisterStatusFlag_InUse);
		fprintf(file, "    mov %s, %d\n", register_name(r, 8), ast[index].data_literal_int.value);
		return r;
	}
	else if (ast[index].type == AstNodeType::LiteralBool)
	{
		int r = registers.get_free_register(RegisterStatusFlag_InUse);
		if (ast[index].data_literal_bool.value)
			fprintf(file, "    mov %s, %d\n", register_name(r, 1), 1);
		else
			fprintf(file, "    mov %s, %d\n", register_name(r, 1), 0);
		return r;
	}
	else if (ast[index].type == AstNodeType::LiteralChar)
	{
		int r = registers.get_free_register(RegisterStatusFlag_InUse);
		fprintf(file, "    mov %s, %d\n", register_name(r, 8), ast[index].data_literal_int.value);
		return r;
	}
	else if (ast[index].type == AstNodeType::LiteralString)
	{
		int r = registers.get_free_register(RegisterStatusFlag_InUse);
		auto str_index = ast[index].data_literal_string.constant_string_index;
		fprintf(file, "    mov %s, qword LSTR%zu\n", register_name(r, 8), str_index);
		return r;
	}
	else if (ast[index].type == AstNodeType::LiteralFloat)
	{
		// Get a temporary register to load address of float constant
		int temp_reg = registers.get_free_register(0);
		auto float_index = ast[index].data_literal_float.constant_float_index;
		fprintf(file, "    mov %s, qword LFLT%zu\n", register_name(temp_reg, 8), float_index);

		int r = registers.get_free_xmm_register(RegisterStatusFlag_InUse);
		fprintf(file, "    movsd %s, [%s]\n", xmm_register_name(r), register_name(temp_reg, 8));
		return r;
	}
	else if (ast[index].type == AstNodeType::BinOpAdd
		  || ast[index].type == AstNodeType::BinOpSub
		  || ast[index].type == AstNodeType::BinOpMul
		  || ast[index].type == AstNodeType::BinOpDiv
		  || ast[index].type == AstNodeType::BinCompGreater
		  || ast[index].type == AstNodeType::BinCompGreaterEqual
		  || ast[index].type == AstNodeType::BinCompLess
		  || ast[index].type == AstNodeType::BinCompLessEqual
		  || ast[index].type == AstNodeType::BinCompEqual
		  || ast[index].type == AstNodeType::BinCompNotEqual
		  || ast[index].type == AstNodeType::BinLogicalAnd
		  || ast[index].type == AstNodeType::BinLogicalOr
		)
	{
		auto& lhs = ast[ast[index].child0];
		auto& rhs = ast[ast[index].child1];

		int r1 = codegen_expr(ast, symbol_table, file, ast[index].child0, registers);
		int r0 = codegen_expr(ast, symbol_table, file, ast[index].child1, registers);

		if (is_float_type(lhs.type_annotation.value()))
		{
			int r2 = codegen_binop_float(ast, index, r0, r1, file, registers);

			registers.register_status[r2].set_all_flags(RegisterStatusFlag_InUse);
			if (r2 != r0) registers.register_status[r0].unset_flag(RegisterStatusFlag_InUse);
			if (r2 != r1) registers.register_status[r1].unset_flag(RegisterStatusFlag_InUse);

			return r2;
		}
		else
		{
			size_t arg_size = 8;
			if (lhs.type_annotation->special == false)
				arg_size = symbol_table.types[lhs.type_annotation->type_index].data_size;
			else if (rhs.type_annotation->special == false)
				arg_size = symbol_table.types[rhs.type_annotation->type_index].data_size;

			codegen_binop(ast, index, r0, r1, arg_size, file, registers);

			registers.register_status[r0].set_all_flags(RegisterStatusFlag_InUse);
			if (r0 != r1) registers.register_status[r1].unset_flag(RegisterStatusFlag_InUse);

			return r0;
		}
	}
	else if (ast[index].type == AstNodeType::Variable || ast[index].type == AstNodeType::Selector)
	{
		auto [stack_offset, data_size] = compute_stack_offset_and_size(ast, symbol_table, index);

		for (int r = 0; r < 32; r++)
		{
			auto& rs = registers.register_status[r];
			if (rs.has_flag(RegisterStatusFlag_ContainsVariable))
			{
				if (rs.stack_offset == stack_offset && rs.stack_size == data_size)
				{
					rs.set_flag(RegisterStatusFlag_InUse);
					return r;
				}
			}
		}

		int r;
		if (is_float_type(ast[index].type_annotation.value()))
		{
			r = registers.get_free_xmm_register(RegisterStatusFlag_InUse | RegisterStatusFlag_ContainsVariable);
			const char* move_ins = is_float_64_type(ast[index].type_annotation.value()) ? "movsd" : "movss";
			fprintf(file, "    %s %s, [rbp - %d]\n", move_ins, xmm_register_name(r), stack_offset);
		}
		else
		{
			r = registers.get_free_register(RegisterStatusFlag_InUse | RegisterStatusFlag_ContainsVariable);
			fprintf(file, "    mov %s, [rbp - %d]\n", register_name(r, data_size), stack_offset);
		}
		registers.register_status[r].stack_offset = stack_offset;
		registers.register_status[r].stack_size = data_size;
		return r;
	}
	else if (ast[index].type == AstNodeType::FunctionCall)
	{
		auto func_index = ast[index].data_function_call.function_index;
		auto& func = symbol_table.functions[func_index];
		auto& func_scope = symbol_table.scopes[func.scope];

		// Save all the registers in use
		for (int i = 0; i < 9; i++)
		{
			int r = caller_saved_registers[i];
			if (registers.register_status[r].has_flag(RegisterStatusFlag_InUse))
			{
				fprintf(file, "    push %s\n", register_name(r, 8));
			}
		}

		if (func.parameters.size() != 0)
		{
			auto current_arg_node = ast[index].child0;

			RegisterState registers_temp = registers;

			int non_float_iter = 0;
			int float_iter = 0;
			for (auto param_variable_index : func.parameters)
			{
				int r = codegen_expr(ast, symbol_table, file, ast[current_arg_node].child0, registers_temp);

				auto var_type = func_scope.local_variables[param_variable_index].type_index;

				TypeAnnotation ta;
				ta.special = false;
				ta.type_index = var_type;

				int param_register;

				if (is_float_type(ta))
				{
					if (is_float_32_type(ta))
					{
						// Might need to convert f64 to f32 because f32 is compatible with float literal
						if (is_float_64_type(ast[ast[current_arg_node].child0].type_annotation.value()))
						{
							fprintf(file, "    cvtsd2ss %s, %s\n", xmm_register_name(r), xmm_register_name(r));
						}
					}

					param_register = xmm_register_for_parameter(float_iter);
					const char* move_ins = is_float_64_type(ta) ? "movsd" : "movss";
					fprintf(file, "    %s %s, %s\n", move_ins, xmm_register_name(param_register), xmm_register_name(r));
					float_iter += 1;
				}
				else
				{
					param_register = register_for_parameter(non_float_iter);
					fprintf(file, "    mov %s, %s\n", register_name(param_register, 8), register_name(r, 8));
					non_float_iter += 1;
				}

				registers_temp.register_status[r].unset_flag(RegisterStatusFlag_InUse);
				registers_temp.register_status[param_register].set_all_flags(RegisterStatusFlag_InUse);

				current_arg_node = ast[current_arg_node].next.value_or(current_arg_node);
			}
		}

		fprintf(file, "    call %s\n", func.name.c_str());

		std::optional<int> return_reg;
		if (func.return_type_index.has_value())
		{
			TypeAnnotation return_ta;
			return_ta.special = false;
			return_ta.type_index = func.return_type_index.value();

			// Move the result into a free register
			if (is_float_type(return_ta))
			{
				return_reg = registers.get_free_xmm_register(RegisterStatusFlag_InUse);
				if (*return_reg != 16)
				{
					const char* move_ins = is_float_64_type(return_ta) ? "movsd" : "movss";
					fprintf(file, "   %s %s, %s\n", move_ins, xmm_register_name(*return_reg), xmm_register_name(16));
				}
			}
			else
			{
				return_reg = registers.get_free_register(RegisterStatusFlag_InUse);
				if (*return_reg != 0)
					fprintf(file, "    mov %s, %s\n", register_name(*return_reg, 8), register_name(0, 8));
			}

		}

		// Restore the registers
		for (int i = 0; i < 9; i++)
		{
			// Iterate backwards
			int r0 = caller_saved_registers[9 - 1 - i];
			if (registers.register_status[r0].has_flag(RegisterStatusFlag_InUse) && !(return_reg.has_value() && *return_reg == r0))
			{
				fprintf(file, "    pop %s\n", register_name(r0, 8));
			}
		}

		// Everything which wasn't saved is trashed
		for (int i = 0; i < 32; i++)
		{
			if (!registers.register_status[i].has_flag(RegisterStatusFlag_InUse))
				registers.register_status[i].set_all_flags(0);
		}

		return return_reg.value_or(0);
	}
	else if (ast[index].type == AstNodeType::AddressOf)
	{
		size_t variable_node_index = ast[index].child0;

		if (ast[variable_node_index].type == AstNodeType::Variable || ast[variable_node_index].type == AstNodeType::Selector)
		{
			auto& scope = symbol_table.scopes[ast[variable_node_index].data_variable.scope_index];
			auto& variable = scope.local_variables[ast[variable_node_index].data_variable.variable_index];

			int r = registers.get_free_register(RegisterStatusFlag_InUse);
			fprintf(file, "    lea %s, [rbp - %d]\n", register_name(r, 8), variable.stack_offset);
			return r;
		}
		else if (ast[variable_node_index].type == AstNodeType::Function)
		{
			auto func_index = ast[variable_node_index].data_function_call.function_index;
			auto& func = symbol_table.functions[func_index];

			int r = registers.get_free_register(RegisterStatusFlag_InUse);
			fprintf(file, "    mov %s, qword %s\n", register_name(r, 8), func.name.c_str());
			return r;
		}
		else
			internal_error("AddressOf non-variable node");
	}
	else if (ast[index].type == AstNodeType::Dereference)
	{
		int r = codegen_expr(ast, symbol_table, file, ast[index].child0, registers);
		fprintf(file, "    mov %s, [%s]\n", register_name(r, 8), register_name(r, 8));

		// If we just dereferenced a pointer variable using the same register, it doesn't contain
		// the pointer variable any more.
		registers.register_status[r].unset_flag(RegisterStatusFlag_ContainsVariable);

		return r;
	}
	else
	{
		internal_error("Unhandled AST node type in code gen (codegen_expr)");
	}
}

void codegen_statement(Ast& ast, SymbolTable& symbol_table, FILE* file, size_t index, size_t function_index, RegisterState& registers)
{
	if (ast[index].type == AstNodeType::Assignment)
	{
		int r = codegen_expr(ast, symbol_table, file, ast[index].child1, registers);

		auto [stack_offset, data_size] = compute_stack_offset_and_size(ast, symbol_table, ast[index].child0);

		if (is_float_type(ast[ast[index].child0].type_annotation.value()))
		{
			if (is_float_32_type(ast[ast[index].child0].type_annotation.value()))
			{
				// Might need to convert f64 to f32 because f32 is compatible with float literal
				if (is_float_64_type(ast[ast[index].child1].type_annotation.value()))
				{
					fprintf(file, "    cvtsd2ss %s, %s\n", xmm_register_name(r), xmm_register_name(r));
				}

				fprintf(file, "    movss [rbp - %d], %s\n", stack_offset, xmm_register_name(r));
			}
			else
			{
				fprintf(file, "    movsd [rbp - %d], %s\n", stack_offset, xmm_register_name(r));
			}
		}
		else
			fprintf(file, "    mov [rbp - %d], %s\n", stack_offset, register_name(r, data_size));

		for (int i = 0; i < 32; i++)
		{
			if (registers.register_status[i].has_flag(RegisterStatusFlag_ContainsVariable)
				&& registers.register_status[i].stack_offset == stack_offset)
			{
				registers.register_status[i].unset_flag(RegisterStatusFlag_ContainsVariable);
			}
		}

		registers.register_status[r].set_all_flags(RegisterStatusFlag_ContainsVariable);
		registers.register_status[r].stack_offset = stack_offset;
		registers.register_status[r].stack_size = data_size;

		if (ast[index].next.has_value())
			codegen_statement(ast, symbol_table, file, ast[index].next.value(), function_index, registers);
	}
	else if (ast[index].type == AstNodeType::ZeroInitialise)
	{
		auto variable_node = ast[index].child0;
		if (ast[variable_node].type != AstNodeType::Variable)
			log_error(ast[variable_node], "Zero initialise only supported for variable");

		auto& scope = symbol_table.scopes[ast[variable_node].data_variable.scope_index];
		auto& variable = scope.local_variables[ast[variable_node].data_variable.variable_index];
		auto& type = symbol_table.types[variable.type_index];
		auto data_size = type.data_size;

		size_t bytes_to_zero = type.data_size;
		uint32_t addr_to_zero = variable.stack_offset;

		// Get a temporary register - 0 flag because we are done with it immediately
		int r = registers.get_free_register(0);
		fprintf(file, "    mov %s, 0\n", register_name(r, 8));

		while (bytes_to_zero != 0)
		{
			int bytes_this_instruction = 1;
			if (bytes_to_zero >= 8) bytes_this_instruction = 8;
			else if (bytes_to_zero >= 4) bytes_this_instruction = 4;
			else if (bytes_to_zero >= 2) bytes_this_instruction = 2;

			fprintf(file, "    mov [rbp - %d], %s\n", addr_to_zero, register_name(r, bytes_this_instruction));
			addr_to_zero -= bytes_this_instruction;
			bytes_to_zero -= bytes_this_instruction;
		}

		if (ast[index].next.has_value())
			codegen_statement(ast, symbol_table, file, ast[index].next.value(), function_index, registers);
	}
	else if (ast[index].type == AstNodeType::ExpressionStatement)
	{
		int r = codegen_expr(ast, symbol_table, file, ast[index].child0, registers);
		registers.register_status[r].unset_flag(RegisterStatusFlag_InUse);

		if (ast[index].next.has_value())
			codegen_statement(ast, symbol_table, file, ast[index].next.value(), function_index, registers);
	}
	else if (ast[index].type == AstNodeType::Return)
	{
		if (ast[index].aux.has_value())
		{
			int r = codegen_expr(ast, symbol_table, file, ast[index].aux.value(), registers);

			if (r != 0)
			{
				registers.register_status[r].unset_flag(RegisterStatusFlag_InUse);
				if (r < 16)
					fprintf(file, "    mov %s, %s\n", register_name(0, 8), register_name(r, 8));
				else
				{
					auto& func = symbol_table.functions[function_index];
					if (!func.return_type_index.has_value())
						internal_error("Missing return type index");

					TypeAnnotation function_ret_ta;
					function_ret_ta.special = false;
					function_ret_ta.type_index = func.return_type_index.value();

					if (is_float_32_type(function_ret_ta))
					{
						// Might need to convert f64 to f32 because f32 is compatible with float literal
						if (is_float_64_type(ast[ast[index].aux.value()].type_annotation.value()))
						{
							fprintf(file, "    cvtsd2ss %s, %s\n", xmm_register_name(r), xmm_register_name(r));
						}
					}

					fprintf(file, "    movq %s, %s\n", xmm_register_name(16), xmm_register_name(r));
				}
			}
		}

		fprintf(file, "    leave\n");
		fprintf(file, "    ret\n");
	}
	else if (ast[index].type == AstNodeType::If)
	{
		// Else branch is stored in aux
		bool else_branch = ast[index].aux.has_value();

		// L0 is used to jump over the if branch
		size_t L0 = symbol_table.functions[function_index].next_label++;
		// L1 is used to jump over the else branch
		size_t L1 = 0;
		if (else_branch)
			L1 = symbol_table.functions[function_index].next_label++;

		// Evaluate the condition
		int r = codegen_expr(ast, symbol_table, file, ast[index].child0, registers);
		registers.register_status[r].unset_flag(RegisterStatusFlag_InUse);
		fprintf(file, "    test %s, %s\n", register_name(r, 1), register_name(r, 1));

		fprintf(file, "    jz .L%zd\n", L0);

		// If branch code
		codegen_statement(ast, symbol_table, file, ast[index].child1, function_index, registers);
		if (else_branch) // If there is an else branch, skip over it
			fprintf(file, "    jmp .L%zd\n", L1);

		// L0 is at the end of the if branch
		fprintf(file, ".L%zd:\n", L0);

		if (else_branch)
		{
			// Else branch code
			codegen_statement(ast, symbol_table, file, ast[index].aux.value(), function_index, registers);

			// L1 is at the end of the else branch
			fprintf(file, ".L%zd:\n", L1);
		}

		if (ast[index].next.has_value())
			codegen_statement(ast, symbol_table, file, ast[index].next.value(), function_index, registers);

	}
	else if (ast[index].type == AstNodeType::While)
	{
		size_t start_label = symbol_table.functions[function_index].next_label++;
		size_t end_label = symbol_table.functions[function_index].next_label++;

		fprintf(file, ".L%zd:\n", start_label);

		// Evaluate the condition
		int r = codegen_expr(ast, symbol_table, file, ast[index].child0, registers);
		registers.register_status[r].unset_flag(RegisterStatusFlag_InUse);
		fprintf(file, "    test %s, %s\n", register_name(r, 1), register_name(r, 1));

		fprintf(file, "    jz .L%zd\n", end_label);

		// Body
		codegen_statement(ast, symbol_table, file, ast[index].child1, function_index, registers);

		fprintf(file, "    jmp .L%zd\n", start_label);
		fprintf(file, ".L%zd:\n", end_label);

		if (ast[index].next.has_value())
			codegen_statement(ast, symbol_table, file, ast[index].next.value(), function_index, registers);
	}
	else if (ast[index].type == AstNodeType::For)
	{
		auto init_node = ast[index].child0;
		auto cond_node = ast[init_node].aux.value();
		auto incr_node = ast[cond_node].aux.value();
		auto body_node = ast[index].child1;

		size_t start_label = symbol_table.functions[function_index].next_label++;
		size_t end_label = symbol_table.functions[function_index].next_label++;

		// Initialiser
		codegen_statement(ast, symbol_table, file, init_node, function_index, registers);

		fprintf(file, ".L%zd:\n", start_label);

		// Evaluate the condition
		int r = codegen_expr(ast, symbol_table, file, cond_node, registers);
		registers.register_status[r].unset_flag(RegisterStatusFlag_InUse);
		fprintf(file, "    test %s, %s\n", register_name(r, 1), register_name(r, 1));

		fprintf(file, "    jz .L%zd\n", end_label);

		// Body
		codegen_statement(ast, symbol_table, file, body_node, function_index, registers);

		// Incrementer
		codegen_statement(ast, symbol_table, file, incr_node, function_index, registers);

		fprintf(file, "    jmp .L%zd\n", start_label);
		fprintf(file, ".L%zd:\n", end_label);

		if (ast[index].next.has_value())
			codegen_statement(ast, symbol_table, file, ast[index].next.value(), function_index, registers);
	}
	else
	{
		internal_error("Unhandled AST node type in code gen (codegen_statement)");
	}
}

void codegen_function(size_t function_index, SymbolTable& symbol_table, FILE* file, const std::string& asm_label)
{
	auto& func = symbol_table.functions[function_index];

	fprintf(file, "%s:\n", asm_label.c_str());

	auto& ast = func.ast;
	auto index = func.ast_node_root;
	auto& func_scope = symbol_table.scopes[func.scope];

	if (ast[index].type != AstNodeType::FunctionDefinition)
		internal_error("Expected root node for function ast to be function definition");

	// Function preamble
	fprintf(file, "    push rbp\n");
	fprintf(file, "    mov rbp, rsp\n");
	fprintf(file, "    sub rsp, %d\n", ast[index].data_function_definition.stack_size);

	RegisterState registers;
	int non_float_iter = 0;
	int float_iter = 0;
	for (auto param_variable_index : func.parameters)
	{
		auto param_offset = func_scope.local_variables[param_variable_index].stack_offset;
		auto var_type = func_scope.local_variables[param_variable_index].type_index;
		auto data_size = symbol_table.types[var_type].data_size;

		TypeAnnotation ta;
		ta.special = false;
		ta.type_index = var_type;

		int param_register;

		if (is_float_type(ta))
		{
			param_register = xmm_register_for_parameter(float_iter);
			const char* move_ins = is_float_64_type(ta) ? "movsd" : "movss";
			fprintf(file, "    %s [rbp - %d], %s\n", move_ins, param_offset, xmm_register_name(param_register));
			float_iter += 1;
		}
		else
		{
			param_register = register_for_parameter(non_float_iter);
			fprintf(file, "    mov [rbp - %d], %s\n", param_offset, register_name(param_register, data_size));
			non_float_iter += 1;
		}

		registers.register_status[param_register].set_all_flags(RegisterStatusFlag_ContainsVariable);
		registers.register_status[param_register].stack_offset = param_offset;
		registers.register_status[param_register].stack_size = data_size;
	}

	if (ast[index].next.has_value())
		codegen_statement(ast, symbol_table, file, ast[index].next.value(), function_index, registers);

	fprintf(file, "    leave\n");
	fprintf(file, "    ret\n");
	
	fprintf(file, "\n");
}

void codegen(SymbolTable& symbol_table, FILE* file, bool is_libc_mode)
{
	// Check that the main function is defined
	bool main_defined = false;
	for (auto& func : symbol_table.functions)
	{
		if (func.name == "main")
		{
			if (!func.return_type_index.has_value())
				log_error(func.ast[func.ast_node_root], "Main function missing return type");

			if (symbol_table.types[func.return_type_index.value()].name != "u64")
				log_error(func.ast[func.ast_node_root], "Main function defined with wrong return type");

			main_defined = true;
			break;
		}
	}
	if (!main_defined) log_general_error("No main function defined");

	const char* entry_point_name;
	const char* libc_entry_point_name;
	const char* write_syscall;
	const char* exit_syscall;

	if (get_platform() == Platform::Linux)
	{
		entry_point_name = "_start";
		libc_entry_point_name = "main";
		write_syscall = "1";
		exit_syscall = "60";
	}
	else if (get_platform() == Platform::MacOS)
	{
		entry_point_name = "start";
		libc_entry_point_name = "_main";
		write_syscall = "0x2000004";
		exit_syscall = "0x2000001";
	}

	if (!is_libc_mode)
		fprintf(file, "    global    %s\n", entry_point_name);
	else
		fprintf(file, "    global    %s\n", libc_entry_point_name);

	for (auto& func : symbol_table.functions)
	{
		if (func.is_external)
		{
			fprintf(file, "    extern    %s\n", func.name.c_str());
		}
	}

	fprintf(file, "\n");
	fprintf(file, "    section   .text\n");
	fprintf(file, "\n");
	if (!is_libc_mode)
	{
		fprintf(file, "%s:\n", entry_point_name);
		fprintf(file, "    call main\n");
		fprintf(file, "    call exit\n");
		fprintf(file, "\n");
	}

	fprintf(file, "; user code\n");
	for (size_t i = 0; i < symbol_table.functions.size(); i++)
	{
		auto& func = symbol_table.functions[i];
		if (!func.intrinsic && !func.is_external)
		{
			if (is_libc_mode && func.name == "main")
				codegen_function(i, symbol_table, file, libc_entry_point_name);
			else
				codegen_function(i, symbol_table, file, func.name);
		}
	}

	fprintf(file, "; intrinsics\n");
	if (!is_libc_mode)
	{
		fprintf(file, "exit:\n");
		fprintf(file, "    mov rax, %s\n", exit_syscall);
		fprintf(file, "    xor rdi, rdi\n");
		fprintf(file, "    syscall\n");
		fprintf(file, "\n");
	}
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
	fprintf(file, "    mov eax, %s\n", write_syscall);
	fprintf(file, "    mov edi, 1\n");
	fprintf(file, "    lea edx, [rsp+16 + 1]\n");
	fprintf(file, "    sub edx, esi\n");
	fprintf(file, "    syscall\n");
	fprintf(file, "    add rsp, 24\n");
	fprintf(file, "    ret\n");

	fprintf(file, "print_bool:\n");
	fprintf(file, "    test dil, dil\n");
	fprintf(file, "    mov       rax, %s\n", write_syscall);
	fprintf(file, "    mov       rdi, 1\n");
	fprintf(file, "    jz .is_zero\n");
	fprintf(file, "    mov       rsi, qword bool_print_true_msg\n");
	fprintf(file, "    mov       rdx, 5\n");
	fprintf(file, "    jmp .print\n");
	fprintf(file, ".is_zero:\n");
	fprintf(file, "    mov       rsi, qword bool_print_false_msg\n");
	fprintf(file, "    mov       rdx, 6\n");
	fprintf(file, ".print:\n");
	fprintf(file, "    syscall\n");
	fprintf(file, "    ret\n");

	fprintf(file, "print_char:\n");
	fprintf(file, "    push rbp\n");
	fprintf(file, "    mov rbp, rsp\n");
	fprintf(file, "    sub rsp, 16\n");
	fprintf(file, "    mov [rsp], dil\n");
	fprintf(file, "    mov rax, 10\n");
	fprintf(file, "    mov [rsp + 1], %s\n", register_name(0, 1));
	fprintf(file, "    mov rax, %s\n", write_syscall);
	fprintf(file, "    mov rdi, 1\n");   // stdout
	fprintf(file, "    mov rsi, rsp\n"); // address
	fprintf(file, "    mov rdx, 2\n");   // length
	fprintf(file, "    syscall\n");
	fprintf(file, "    leave\n");
	fprintf(file, "    ret\n");

	fprintf(file, "print_string:\n");
	fprintf(file, "    push rbp\n");
	fprintf(file, "    mov rbp, rsp\n");

	fprintf(file, "    mov rsi, rdi\n"); // address

	// Get length of string in rdx
	fprintf(file, "    mov rdx, 0\n");
	fprintf(file, ".loop:\n");
	fprintf(file, "    mov rax, [rsi + rdx]\n");
	fprintf(file, "    add rdx, 1\n");
	fprintf(file, "    cmp al, 10\n");
	fprintf(file, "    jne .loop\n");

	fprintf(file, "    mov rax, %s\n", write_syscall);
	fprintf(file, "    mov rdi, 1\n");   // stdout
	fprintf(file, "    syscall\n");
	fprintf(file, "    leave\n");
	fprintf(file, "    ret\n");

	fprintf(file, "itoa:\n"); // rdi = integer, rsi = address to write
	fprintf(file, "    push rbp\n");
	fprintf(file, "    mov rbp, rsp\n");
	fprintf(file, "    sub rsp, 16\n");

	fprintf(file, "    mov eax, edi\n");
	fprintf(file, "    mov ecx, 10\n");

	fprintf(file, "    mov rdi, rsi\n");
	fprintf(file, "    mov rsi, rbp\n");

	fprintf(file, ".toascii_digit:\n");
	fprintf(file, "    xor edx, edx\n");
	fprintf(file, "    div ecx\n");
	fprintf(file, "    add edx, '0'\n");
	fprintf(file, "    dec rsi\n");
	fprintf(file, "    mov [rsi], dl\n");
	fprintf(file, "    test eax, eax\n");
	fprintf(file, "    jnz .toascii_digit\n");

	// Write the buffer back to rdi (original rsi)
	fprintf(file, "    mov rcx, 0\n");
	fprintf(file, ".loop:\n");
	fprintf(file, "    mov rax, [rsi]\n");
	fprintf(file, "    mov [rdi], rax\n");
	fprintf(file, "    inc rsi\n");
	fprintf(file, "    inc rdi\n");
	fprintf(file, "    inc rcx\n");
	fprintf(file, "    cmp rsi, rbp\n");
	fprintf(file, "    jne .loop\n");

	fprintf(file, "    mov rax, rcx\n");

	fprintf(file, "    leave\n");
	fprintf(file, "    ret\n");

	fprintf(file, "print_float:\n");
	fprintf(file, "    push rbp\n");
	fprintf(file, "    mov rbp, rsp\n");
	fprintf(file, "    sub rsp, 64\n");

	fprintf(file, "    cvttsd2si rdi, xmm0\n");
	// xmm1 = integer part
	fprintf(file, "    cvtsi2sd xmm1, rdi\n");
	// xmm0 = fractional part
	fprintf(file, "    subsd xmm0, xmm1\n");

	fprintf(file, "    mov rsi, rsp\n");
	fprintf(file, "    call itoa\n");

	fprintf(file, "    mov r8, rax\n");

	fprintf(file, "    mov BYTE [rsp + r8], 46\n");
	fprintf(file, "    inc r8\n");

	// xmm2 = 10
	fprintf(file, "    mov rax, 10\n");
	fprintf(file, "    xorps xmm2, xmm2\n"); // Clear xmm2
	fprintf(file, "    cvtsi2sd xmm2, rax\n");

	// i = 0
	fprintf(file, "    mov rax, 0\n");
	fprintf(file, ".loop\n");
	fprintf(file, "    mulsd xmm0, xmm2\n");
	fprintf(file, "    cvttsd2si rcx, xmm0\n"); // Integer part
	fprintf(file, "    xorps xmm1, xmm1\n"); // Clear xmm1
	fprintf(file, "    cvtsi2sd xmm1, rcx\n"); // xmm1 is integer part
	fprintf(file, "    subsd xmm0, xmm1\n"); // xmm0 -= xmm1

	// rcx has the digit
	fprintf(file, "    add cl, 48\n"); // add '0'
	fprintf(file, "    mov [rsp + r8], cl\n");
	fprintf(file, "    inc rax\n");
	fprintf(file, "    inc r8\n");
	fprintf(file, "    cmp rax, 6\n");
	fprintf(file, "    jne .loop\n");

	fprintf(file, "    mov BYTE [rsp + r8], 10\n");
	fprintf(file, "    inc r8\n");

	// print the buffer
	fprintf(file, "    mov rsi, rsp\n");
	fprintf(file, "    mov rdx, r8\n");
	fprintf(file, "    mov rax, %s\n", write_syscall);
	fprintf(file, "    mov rdi, 1\n");
	fprintf(file, "    syscall\n");

	fprintf(file, "    leave\n");
	fprintf(file, "    ret\n");

	fprintf(file, "print_float32:\n");
	fprintf(file, "    push rbp\n");
	fprintf(file, "    mov rbp, rsp\n");
	fprintf(file, "    cvtss2sd xmm0, xmm0\n");
	fprintf(file, "    call print_float\n");
	fprintf(file, "    leave\n");
	fprintf(file, "    ret\n");

	fprintf(file, "    section .data\n");
	fprintf(file, "bool_print_true_msg:  db        \"true\", 10\n");
	fprintf(file, "bool_print_false_msg:  db        \"false\", 10\n");

	for (size_t i = 0; i < symbol_table.constant_strings.size(); i++)
	{
		fprintf(file, "LSTR%zu: db \"%s\", 10\n", i, symbol_table.constant_strings[i].str.c_str());
	}

	for (size_t i = 0; i < symbol_table.constant_floats.size(); i++)
	{
		char flt_str[64];
		snprintf(flt_str, 64, "%f", symbol_table.constant_floats[i]);
		fprintf(file, "LFLT%zu: dq %s\n", i, flt_str);
	}
}
