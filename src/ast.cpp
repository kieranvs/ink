#include "ast.h"

#include "utils.h"

#include <stdio.h>

void dump_ast(FILE* output, SymbolTable& symbol_table, Ast& ast, size_t index, int indent)
{
	for (int i = 0; i < indent; i++)
		fprintf(output, "  ");

	if (ast[index].type == AstNodeType::None)
		return;
	else if (ast[index].type == AstNodeType::LiteralInt)
		fprintf(output, "%d\n", ast[index].data_literal_int.value);
	else if (ast[index].type == AstNodeType::LiteralBool)
		fprintf(output, "%s\n", ast[index].data_literal_bool.value ? "true" : "false");
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
		if (ast[index].type == AstNodeType::BinOpAdd) fprintf(output, "+\n");
		if (ast[index].type == AstNodeType::BinOpMul) fprintf(output, "*\n");
		if (ast[index].type == AstNodeType::BinCompGreater) fprintf(output, ">\n");
		if (ast[index].type == AstNodeType::BinCompGreaterEqual) fprintf(output, ">=\n");
		if (ast[index].type == AstNodeType::BinCompLess) fprintf(output, "<\n");
		if (ast[index].type == AstNodeType::BinCompLessEqual) fprintf(output, "<=\n");
		if (ast[index].type == AstNodeType::BinCompEqual) fprintf(output, "==\n");
		if (ast[index].type == AstNodeType::BinCompNotEqual) fprintf(output, "!=\n");

		dump_ast(output, symbol_table, ast, ast[index].child0, indent + 1);
		dump_ast(output, symbol_table, ast, ast[index].child1, indent + 1);
	}
	else if (ast[index].type == AstNodeType::Variable)
	{
		fprintf(output, "Variable %d\n", ast[index].data_variable.variable_index);
	}
	else if (ast[index].type == AstNodeType::Assignment)
	{
		fprintf(output, "=\n");
		dump_ast(output, symbol_table, ast, ast[index].child0, indent + 1);
		dump_ast(output, symbol_table, ast, ast[index].child1, indent + 1);
		if (ast[index].next.has_value())
			dump_ast(output, symbol_table, ast, ast[index].next.value(), indent);
	}
	else if (ast[index].type == AstNodeType::Return)
	{
		fprintf(output, "return\n");
		dump_ast(output, symbol_table, ast, ast[index].child0, indent + 1);
	}
	else if (ast[index].type == AstNodeType::ExpressionStatement)
	{
		fprintf(output, "Expr statement\n");
		dump_ast(output, symbol_table, ast, ast[index].child0, indent + 1);
		if (ast[index].next.has_value())
			dump_ast(output, symbol_table, ast, ast[index].next.value(), indent);
	}
	else if (ast[index].type == AstNodeType::FunctionDefinition)
	{
		fprintf(output, "Function\n");
		if (ast[index].next.has_value())
			dump_ast(output, symbol_table, ast, ast[index].next.value(), indent + 1);
	}
	else if (ast[index].type == AstNodeType::FunctionCall)
	{
		fprintf(output, "Function call %d\n", ast[index].data_function_call.function_index);

		auto func_index = ast[index].data_function_call.function_index;
		auto& func = symbol_table.functions[func_index];
		auto& func_scope = symbol_table.scopes[func.scope];

		if (func.parameters.size() != 0)
		{
			auto current_arg_node = ast[index].child0;

			for (int i = 0; i < func.parameters.size(); i++)
			{
				dump_ast(output, symbol_table, ast, current_arg_node, indent + 1);
				current_arg_node = ast[current_arg_node].next.value_or(current_arg_node);
			}
		}

	}
	else if (ast[index].type == AstNodeType::FunctionCallArg)
	{
		fprintf(output, "Function call arg\n");
		dump_ast(output, symbol_table, ast, ast[index].child0, indent + 1);
	}
	else
	{
		internal_error("Unhandled AST node type in dump_ast");
	}
}

std::optional<std::pair<size_t, size_t>> SymbolTable::find_variable(size_t scope_index, const std::string& name)
{
	auto& scope = scopes[scope_index];

	for (size_t i = 0; i < scope.local_variables.size(); i++)
	{
		if (scope.local_variables[i].name == name) return std::make_pair(scope_index, i);
	}

	if (scope.parent.has_value())
		return find_variable(scope.parent.value(), name);

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
		v.scope_offset = type.data_size;
	else
		v.scope_offset = local_variables[local_variables.size() - 2].scope_offset + type.data_size;

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