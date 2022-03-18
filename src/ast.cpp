#include "ast.h"

#include "utils.h"

#include <stdio.h>

void dump_ast(SymbolTable& symbol_table, Ast& ast, size_t index, int indent)
{
	for (int i = 0; i < indent; i++)
		printf("  ");

	if (ast[index].type == AstNodeType::None)
		return;
	else if (ast[index].type == AstNodeType::LiteralInt)
		printf("%d\n", ast[index].data_literal_int.value);
	else if (ast[index].type == AstNodeType::LiteralBool)
		printf("%s\n", ast[index].data_literal_bool.value ? "true" : "false");
	else if (ast[index].type == AstNodeType::BinOpAdd
		  || ast[index].type == AstNodeType::BinOpMul
		  || ast[index].type == AstNodeType::BinCompGreater
		  || ast[index].type == AstNodeType::BinCompGreaterEqual
		  || ast[index].type == AstNodeType::BinCompLess
		  || ast[index].type == AstNodeType::BinCompLessEqual
		  || ast[index].type == AstNodeType::BinCompEqual
		  || ast[index].type == AstNodeType::BinCompNotEqual
		)
	{
		if (ast[index].type == AstNodeType::BinOpAdd) printf("+\n");
		if (ast[index].type == AstNodeType::BinOpMul) printf("*\n");
		if (ast[index].type == AstNodeType::BinCompGreater) printf(">\n");
		if (ast[index].type == AstNodeType::BinCompGreaterEqual) printf(">=\n");
		if (ast[index].type == AstNodeType::BinCompLess) printf("<\n");
		if (ast[index].type == AstNodeType::BinCompLessEqual) printf("<=\n");
		if (ast[index].type == AstNodeType::BinCompEqual) printf("==\n");
		if (ast[index].type == AstNodeType::BinCompNotEqual) printf("!=\n");

		dump_ast(symbol_table, ast, ast[index].child0, indent + 1);
		dump_ast(symbol_table, ast, ast[index].child1, indent + 1);
	}
	else if (ast[index].type == AstNodeType::Variable)
	{
		printf("Variable %d\n", ast[index].data_variable.variable_index);
	}
	else if (ast[index].type == AstNodeType::Assignment)
	{
		printf("=\n");
		dump_ast(symbol_table, ast, ast[index].child0, indent + 1);
		dump_ast(symbol_table, ast, ast[index].child1, indent + 1);
		if (ast[index].next.has_value())
			dump_ast(symbol_table, ast, ast[index].next.value(), indent);
	}
	else if (ast[index].type == AstNodeType::Return)
	{
		printf("return\n");
		dump_ast(symbol_table, ast, ast[index].child0, indent + 1);
	}
	else if (ast[index].type == AstNodeType::ExpressionStatement)
	{
		printf("Expr statement\n");
		dump_ast(symbol_table, ast, ast[index].child0, indent + 1);
		if (ast[index].next.has_value())
			dump_ast(symbol_table, ast, ast[index].next.value(), indent);
	}
	else if (ast[index].type == AstNodeType::FunctionDefinition)
	{
		printf("Function\n");
		if (ast[index].next.has_value())
			dump_ast(symbol_table, ast, ast[index].next.value(), indent + 1);
	}
	else if (ast[index].type == AstNodeType::FunctionCall)
	{
		printf("Function call %d\n", ast[index].data_function_call.function_index);

		auto func_index = ast[index].data_function_call.function_index;
		auto& func = symbol_table.functions[func_index];
		auto& func_scope = symbol_table.scopes[func.scope];

		if (func.parameters.size() != 0)
		{
			auto current_arg_node = ast[index].child0;

			for (int i = 0; i < func.parameters.size(); i++)
			{
				dump_ast(symbol_table, ast, current_arg_node, indent + 1);
				current_arg_node = ast[current_arg_node].next.value_or(current_arg_node);
			}
		}

	}
	else if (ast[index].type == AstNodeType::FunctionCallArg)
	{
		printf("Function call arg\n");
		dump_ast(symbol_table, ast, ast[index].child0, indent + 1);
	}
	else
	{
		internal_error("Unhandled AST node type in dump_ast");
	}
}

std::optional<size_t> Scope::find_variable(const std::string& name)
{
	for (size_t i = 0; i < local_variables.size(); i++)
	{
		if (local_variables[i].name == name) return i;
	}

	return std::nullopt;
}

std::optional<size_t> Scope::make_variable(SymbolTable& symbol_table, const std::string& name, size_t type_index)
{
	for (size_t i = 0; i < local_variables.size(); i++)
	{
		// Error condition
		if (local_variables[i].name == name) return std::nullopt;
	}

	auto& type = symbol_table.types[type_index];

	local_variables.emplace_back();
	auto& v = local_variables.back();

	v.name = name;
	v.type_index = type_index;
	if (local_variables.size() == 1)
		v.stack_offset = type.data_size;
	else
		v.stack_offset = local_variables[local_variables.size() - 2].stack_offset + type.data_size;

	return local_variables.size() - 1;
}

std::optional<size_t> SymbolTable::find_function(const std::string& name)
{
	for (size_t i = 0; i < functions.size(); i++)
	{
		if (functions[i].name == name)
			return i;
	}

	return std::nullopt;
}

std::optional<size_t> SymbolTable::find_type(const std::string& name)
{
	for (size_t i = 0; i < types.size(); i++)
	{
		if (types[i].name == name)
			return i;
	}

	return std::nullopt;
}