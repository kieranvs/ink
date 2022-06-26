#include "ast.h"

#include "errors.h"
#include "utils.h"

#include <stdio.h>

void dump_ast(FILE* output, SymbolTable& symbol_table, Ast& ast, size_t index, int indent = 0)
{
	for (int i = 0; i < indent; i++)
		fprintf(output, "  ");

	if (ast[index].type == AstNodeType::None)
		return;
	else if (ast[index].type == AstNodeType::LiteralInt)
		fprintf(output, "%d\n", ast[index].data_literal_int.value);
	else if (ast[index].type == AstNodeType::LiteralBool)
		fprintf(output, "%s\n", ast[index].data_literal_bool.value ? "true" : "false");
	else if (ast[index].type == AstNodeType::LiteralChar)
		fprintf(output, "\'%c\'\n", ast[index].data_literal_int.value);
	else if (ast[index].type == AstNodeType::LiteralString)
	{
		auto& str = symbol_table.constant_strings[ast[index].data_literal_string.constant_string_index].str;
		fprintf(output, "\"%s\"\n", str.c_str());
	}
	else if (ast[index].type == AstNodeType::LiteralFloat)
	{
		auto& val = symbol_table.constant_floats[ast[index].data_literal_float.constant_float_index];
		fprintf(output, "%f\n", val);
	}
	else if (ast[index].type == AstNodeType::Dereference
		|| ast[index].type == AstNodeType::AddressOf)
	{
		if (ast[index].type == AstNodeType::Dereference) fprintf(output, "Dereference\n");
		if (ast[index].type == AstNodeType::AddressOf) fprintf(output, "AddressOf\n");

		dump_ast(output, symbol_table, ast, ast[index].child0, indent + 1);
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
		if (ast[index].type == AstNodeType::BinOpAdd) fprintf(output, "+\n");
		if (ast[index].type == AstNodeType::BinOpSub) fprintf(output, "-\n");
		if (ast[index].type == AstNodeType::BinOpMul) fprintf(output, "*\n");
		if (ast[index].type == AstNodeType::BinOpDiv) fprintf(output, "/\n");
		if (ast[index].type == AstNodeType::BinCompGreater) fprintf(output, ">\n");
		if (ast[index].type == AstNodeType::BinCompGreaterEqual) fprintf(output, ">=\n");
		if (ast[index].type == AstNodeType::BinCompLess) fprintf(output, "<\n");
		if (ast[index].type == AstNodeType::BinCompLessEqual) fprintf(output, "<=\n");
		if (ast[index].type == AstNodeType::BinCompEqual) fprintf(output, "==\n");
		if (ast[index].type == AstNodeType::BinCompNotEqual) fprintf(output, "!=\n");
		if (ast[index].type == AstNodeType::BinLogicalAnd) fprintf(output, "&&\n");
		if (ast[index].type == AstNodeType::BinLogicalOr) fprintf(output, "||\n");

		dump_ast(output, symbol_table, ast, ast[index].child0, indent + 1);
		dump_ast(output, symbol_table, ast, ast[index].child1, indent + 1);
	}
	else if (ast[index].type == AstNodeType::Variable)
	{
		auto& variable = symbol_table.scopes[ast[index].data_variable.scope_index].local_variables[ast[index].data_variable.variable_index];
		fprintf(output, "Variable %s\n", variable.name.c_str());
	}
	else if (ast[index].type == AstNodeType::VariableGlobal)
	{
		auto& variable = symbol_table.global_variables[ast[index].data_variable.variable_index];
		fprintf(output, "Global Variable %s\n", variable.name.c_str());
	}
	else if (ast[index].type == AstNodeType::Selector)
	{
		auto& variable = symbol_table.scopes[ast[index].data_variable.scope_index].local_variables[ast[index].data_variable.variable_index];
		fprintf(output, "Selector %s\n", variable.name.c_str());

		dump_ast(output, symbol_table, ast, ast[index].child0, indent + 1);
	}
	else if (ast[index].type == AstNodeType::Assignment)
	{
		fprintf(output, "=\n");
		dump_ast(output, symbol_table, ast, ast[index].child0, indent + 1);
		dump_ast(output, symbol_table, ast, ast[index].child1, indent + 1);
		if (ast[index].next.has_value())
			dump_ast(output, symbol_table, ast, ast[index].next.value(), indent);
	}
	else if (ast[index].type == AstNodeType::ZeroInitialise)
	{
		fprintf(output, "Zero initialise\n");
		dump_ast(output, symbol_table, ast, ast[index].child0, indent + 1);
		if (ast[index].next.has_value())
			dump_ast(output, symbol_table, ast, ast[index].next.value(), indent);
	}
	else if (ast[index].type == AstNodeType::Return)
	{
		fprintf(output, "return\n");
		if (ast[index].aux.has_value())
			dump_ast(output, symbol_table, ast, ast[index].aux.value(), indent + 1);
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
		auto func_index = ast[index].data_function_call.function_index;
		auto& func = symbol_table.functions[func_index];

		fprintf(output, "Function call %s\n", func.name.c_str());

		if (func.parameters.size() != 0)
		{
			auto current_arg_node = ast[index].child0;

			for (size_t i = 0; i < func.parameters.size(); i++)
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
	else if (ast[index].type == AstNodeType::Function)
	{
		auto func_index = ast[index].data_function_call.function_index;
		auto& func = symbol_table.functions[func_index];

		fprintf(output, "Function %s\n", func.name.c_str());
	}
	else if (ast[index].type == AstNodeType::If)
	{
		fprintf(output, "If\n");
		dump_ast(output, symbol_table, ast, ast[index].child0, indent + 1);

		for (int i = 0; i < indent; i++)
			fprintf(output, "  ");
		fprintf(output, "Then\n");
		dump_ast(output, symbol_table, ast, ast[index].child1, indent + 1);

		if (ast[index].aux.has_value())
		{
			for (int i = 0; i < indent; i++)
				fprintf(output, "  ");
			fprintf(output, "Else\n");
			dump_ast(output, symbol_table, ast, ast[index].aux.value(), indent + 1);
		}

		if (ast[index].next.has_value())
			dump_ast(output, symbol_table, ast, ast[index].next.value(), indent);
	}
	else if (ast[index].type == AstNodeType::While)
	{
		fprintf(output, "While\n");
		dump_ast(output, symbol_table, ast, ast[index].child0, indent + 1);

		for (int i = 0; i < indent; i++)
			fprintf(output, "  ");
		fprintf(output, "Do\n");
		dump_ast(output, symbol_table, ast, ast[index].child1, indent + 1);

		if (ast[index].next.has_value())
			dump_ast(output, symbol_table, ast, ast[index].next.value(), indent);
	}
	else if (ast[index].type == AstNodeType::For)
	{
		auto init_node = ast[index].child0;
		auto cond_node = ast[init_node].aux.value();
		auto incr_node = ast[cond_node].aux.value();
		auto body_node = ast[index].child1;

		fprintf(output, "For\n");

		for (int i = 0; i < indent; i++)
			fprintf(output, "  ");
		fprintf(output, "Init\n");
		dump_ast(output, symbol_table, ast, init_node, indent + 1);

		for (int i = 0; i < indent; i++)
			fprintf(output, "  ");
		fprintf(output, "While\n");
		dump_ast(output, symbol_table, ast, cond_node, indent + 1);

		for (int i = 0; i < indent; i++)
			fprintf(output, "  ");
		fprintf(output, "Increment\n");
		dump_ast(output, symbol_table, ast, incr_node, indent + 1);

		for (int i = 0; i < indent; i++)
			fprintf(output, "  ");
		fprintf(output, "Do\n");
		dump_ast(output, symbol_table, ast, body_node, indent + 1);

		if (ast[index].next.has_value())
			dump_ast(output, symbol_table, ast, ast[index].next.value(), indent);
	}
	else
	{
		printf("type=%d\n", ast[index].type);
		internal_error("Unhandled AST node type in dump_ast");
	}
}

void pretty_print_type(FILE* output, SymbolTable& symbol_table, size_t type_index)
{
	auto& t = symbol_table.types[type_index];

	if (t.type == TypeType::Intrinsic || t.type == TypeType::Struct)
		fprintf(output, "%s", t.name.c_str());
	else if (t.type == TypeType::Pointer)
	{
		pretty_print_type(output, symbol_table, t.remove_ptr_type);
		fprintf(output, "*");
	}
	else if (t.type == TypeType::Alias)
	{
		fprintf(output, "%s=", t.name.c_str());
		pretty_print_type(output, symbol_table, t.actual_type);
	}
	else if (t.type == TypeType::Function)
	{
		auto& function_type = symbol_table.function_types[t.function_type_index];

		fprintf(output, "(");
		bool first = true;
		for (auto param_type : function_type.parameter_types)
		{
			if (!first) fprintf(output, ", ");
			pretty_print_type(output, symbol_table, param_type);
			first = false;
		}
		fprintf(output, ")");
		if (function_type.return_type_index.has_value())
		{
			fprintf(output, " -> ");
			pretty_print_type(output, symbol_table, function_type.return_type_index.value());
		}
	}
	else if (t.type == TypeType::Incomplete)
	{
		fprintf(output, "<incomplete type>");
	}
	else
		internal_error("Unhandled type type in pretty_print_type\n");
}

void dump_scope(FILE* output, SymbolTable& symbol_table, size_t index, int indent = 0)
{
	for (auto& variable : symbol_table.scopes[index].local_variables)
	{
		for (int i = 0; i < indent; i++)
			fprintf(output, "  ");
		fprintf(output, "%s (type=", variable.name.c_str());
		pretty_print_type(output, symbol_table, variable.type_index);
		fprintf(output, ", stack_offset=%u)\n", variable.stack_offset);
	}

	for (size_t child = 0; child < symbol_table.scopes.size(); child++)
	{
		if (symbol_table.scopes[child].parent.has_value() && symbol_table.scopes[child].parent.value() == index)
		{
			dump_scope(output, symbol_table, child, indent + 1);
		}
	}
}

void dump_symbol_table(FILE* output, SymbolTable& symbol_table)
{
	fprintf(output, "------ Types ------\n");
	for (size_t i = 0; i < symbol_table.types.size(); i++)
	{
		auto& t = symbol_table.types[i];

		fprintf(output, "%zu (", i);
		pretty_print_type(output, symbol_table, i);
		fprintf(output, ")\n");

		if (t.type == TypeType::Intrinsic)
			fprintf(output, "  Type: Intrinsic\n");
		else if (t.type == TypeType::Pointer)
			fprintf(output, "  Type: Pointer\n");
		else if (t.type == TypeType::Struct)
			fprintf(output, "  Type: Struct\n");
		else if (t.type == TypeType::Alias)
			fprintf(output, "  Type: Alias\n");
		else if (t.type == TypeType::Function)
			fprintf(output, "  Type: Function Type\n");
		else
			internal_error("Unhandled type type in dump_symbol_table\n");

		fprintf(output, "  Size: %zd\n", t.data_size);

		if (t.type == TypeType::Struct)
		{
			fprintf(output, "  Scope:\n");
			dump_scope(output, symbol_table, t.scope, 2);
		}
	}

	fprintf(output, "\n------ Global Variables ------\n");
	for (auto& variable : symbol_table.global_variables)
	{
		fprintf(output, "%s (type=", variable.name.c_str());
		pretty_print_type(output, symbol_table, variable.type_index);
		fprintf(output, ")\n");
	}

	fprintf(output, "\n------ Functions ------\n");
	for (auto& func : symbol_table.functions)
	{
		fprintf(output, "Function %s\n", func.name.c_str());
		fprintf(output, "Parameters: ");
		for (auto x : func.parameters)
			fprintf(output, "%zd ", x);
		fprintf(output, "\n");

		fprintf(output, "Scope:\n");
		dump_scope(output, symbol_table, func.scope, 1);

		if (!func.intrinsic && !func.is_external)
		{
			fprintf(output, "Ast:\n");
			dump_ast(output, symbol_table, func.ast, func.ast_node_root, 1);
		}

		fprintf(output, "\n");
	}

	fprintf(output, "------ Constant Strings ------\n");
	for (auto& cs : symbol_table.constant_strings)
	{
		fprintf(output, "  \"%s\"", cs.str.c_str());
	}

}

std::optional<VariableFindResult> SymbolTable::find_variable(size_t scope_index, const std::string& name)
{
	auto& scope = scopes[scope_index];

	for (size_t i = 0; i < scope.local_variables.size(); i++)
	{
		if (scope.local_variables[i].name == name)
		{
			VariableFindResult vfr;
			vfr.is_global = false;
			vfr.scope_index = scope_index;
			vfr.variable_index = i;
			return vfr;
		}
	}

	if (scope.parent.has_value())
		return find_variable(scope.parent.value(), name);
	else
	{
		for (size_t i = 0; i < global_variables.size(); i++)
		{
			if (global_variables[i].name == name)
			{
				VariableFindResult vfr;
				vfr.is_global = true;
				vfr.variable_index = i;
				return vfr;
			}
		}
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
	if (type.type == TypeType::Alias)
		internal_error("make_variable called with alias type");

	auto& v = local_variables.emplace_back();
	v.name = name;
	v.type_index = type_index;
	
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
		{
			if (types[i].type == TypeType::Alias)
				return types[i].actual_type;
			else
				return i;
		}
	}

	return std::nullopt;
}

size_t SymbolTable::find_add_type(const std::string& name, const Token& token)
{
	for (size_t i = 0; i < types.size(); i++)
	{
		if (types[i].name == name)
		{
			if (types[i].type == TypeType::Alias)
				return types[i].actual_type;
			else
				return i;
		}
	}

	auto& type = types.emplace_back();
	type.type = TypeType::Incomplete;
	type.name = name;
	type.location = token.location;

	return types.size() - 1;
}

std::optional<size_t> SymbolTable::find_matching_function_type(const std::vector<size_t>& parameter_types, const std::optional<size_t>& return_type)
{
	for (size_t type_index = 0; type_index < types.size(); type_index++)
	{
		bool matches = [&]()
		{
			auto& type = types[type_index];

			if (type.type != TypeType::Function) return false;

			auto& function_type = function_types[type.function_type_index];

			if (function_type.parameter_types.size() != parameter_types.size()) return false;

			for (size_t i = 0; i < parameter_types.size(); i++)
				if (function_type.parameter_types[i] != parameter_types[i]) return false;

			if (function_type.return_type_index != return_type) return false;

			return true;
		}();

		if (matches)
			return type_index;
	}

	return std::nullopt;
}

size_t SymbolTable::find_add_string(const std::string& str)
{
	for (size_t i = 0; i < constant_strings.size(); i++)
	{
		if (constant_strings[i].str == str)
			return i;
	}

	constant_strings.emplace_back(str);
	return constant_strings.size() - 1;
}

size_t SymbolTable::find_add_float(double value)
{
	for (size_t i = 0; i < constant_floats.size(); i++)
	{
		if (constant_floats[i] == value)
			return i;
	}

	constant_floats.emplace_back(value);
	return constant_floats.size() - 1;
}

void SymbolTable::add_linker_path(const std::string& path, bool is_macos_framework)
{
	for (auto& linker_path : linker_paths)
	{
		if (linker_path.path == path)
			return;
	}

	linker_paths.emplace_back(path, is_macos_framework);
}

size_t get_type_add_pointer(SymbolTable& symbol_table, size_t base_type)
{
	if (symbol_table.types[base_type].add_ptr_type.has_value())
		return symbol_table.types[base_type].add_ptr_type.value();
	else
	{
		symbol_table.types.emplace_back();
		auto& type = symbol_table.types.back();
		type.type = TypeType::Pointer;
		type.data_size = 8;
		type.remove_ptr_type = base_type;

		auto new_type_index = symbol_table.types.size() - 1;
		symbol_table.types[base_type].add_ptr_type = new_type_index;

		return new_type_index;
	}
}

size_t get_type_remove_pointer(SymbolTable& symbol_table, size_t ptr_type)
{
	if (symbol_table.types[ptr_type].type != TypeType::Pointer)
		internal_error("Remove pointer from non-pointer type");
	else
		return symbol_table.types[ptr_type].remove_ptr_type;
}