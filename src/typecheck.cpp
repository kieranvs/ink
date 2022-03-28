#include "typecheck.h"

#include "utils.h"

size_t special_type_index_none = 0;
size_t special_type_index_literal_int = 1;
size_t special_type_index_literal_bool = 2;
size_t intrinsic_type_index_int;
size_t intrinsic_type_index_bool;

struct TypeAnnotation
{
	size_t type_index;
	bool special;
};

TypeAnnotation invalid_type_annotation = [](size_t x){
	TypeAnnotation ta;
	ta.type_index = x;
	ta.special = true;
	return ta;
}(special_type_index_none);

bool special_matches(size_t special, size_t actual)
{
	if (special == special_type_index_literal_int)
		return actual == intrinsic_type_index_int;
	else if (special == special_type_index_literal_bool)
		return actual == intrinsic_type_index_bool;

	return false;
}

bool can_assign(TypeAnnotation& to, TypeAnnotation& from)
{
	if (to.special) return false;

	if (from.special)
	{
		return special_matches(from.type_index, to.type_index);
	}
	else
		return to.type_index == from.type_index;
}

bool can_combine(TypeAnnotation& lhs, TypeAnnotation& rhs, TypeAnnotation& expr_ta)
{
	if (lhs.special == rhs.special)
	{
		if (lhs.type_index == rhs.type_index)
		{
			expr_ta = lhs;
			return true;
		}
		else
			return false;
	}
	else if (lhs.special)
	{
		if (special_matches(lhs.type_index, rhs.type_index))
		{
			expr_ta = rhs;
			return true;
		}
		else
			return false;
	}
	else if (rhs.special)
	{
		if (special_matches(rhs.type_index, lhs.type_index))
		{
			expr_ta = lhs;
			return true;
		}
		else
			return false;
	}
}

TypeAnnotation type_check_ast(SymbolTable& symbol_table, Ast& ast, size_t index, size_t return_type_index)
{
	if (ast[index].type == AstNodeType::LiteralInt)
	{
		TypeAnnotation ta;
		ta.type_index = special_type_index_literal_int;
		ta.special = true;
		return ta;
	}
	else if (ast[index].type == AstNodeType::LiteralBool)
	{
		TypeAnnotation ta;
		ta.type_index = special_type_index_literal_bool;
		ta.special = true;
		return ta;
	}
	else if (ast[index].type == AstNodeType::BinOpAdd || ast[index].type == AstNodeType::BinOpMul)
	{
		auto lhs_ta = type_check_ast(symbol_table, ast, ast[index].child0, return_type_index);
		auto rhs_ta = type_check_ast(symbol_table, ast, ast[index].child1, return_type_index);

		TypeAnnotation expr_ta;
		if (!can_combine(lhs_ta, rhs_ta, expr_ta))
			log_error("Type mismatch");

		return expr_ta;
	}
	else if (ast[index].type == AstNodeType::BinCompGreater
		  || ast[index].type == AstNodeType::BinCompGreaterEqual
		  || ast[index].type == AstNodeType::BinCompLess
		  || ast[index].type == AstNodeType::BinCompLessEqual
		  || ast[index].type == AstNodeType::BinCompEqual
		  || ast[index].type == AstNodeType::BinCompNotEqual
		  )
	{
		auto lhs_ta = type_check_ast(symbol_table, ast, ast[index].child0, return_type_index);
		auto rhs_ta = type_check_ast(symbol_table, ast, ast[index].child1, return_type_index);

		TypeAnnotation expr_ta;
		if (!can_combine(lhs_ta, rhs_ta, expr_ta))
			log_error("Type mismatch");

		// Comparison makes bool
		expr_ta.special = false;
		expr_ta.type_index = intrinsic_type_index_bool;

		return expr_ta;
	}
	else if (ast[index].type == AstNodeType::Variable)
	{
		auto& scope = symbol_table.scopes[ast[index].data_variable.scope_index];
		auto variable_index = ast[index].data_variable.variable_index;
		auto& type_index = scope.local_variables[variable_index].type_index;

		TypeAnnotation ta;
		ta.type_index = type_index;
		ta.special = false;
		return ta;
	}
	else if (ast[index].type == AstNodeType::Assignment)
	{
		auto variable_ta = type_check_ast(symbol_table, ast, ast[index].child0, return_type_index);
		auto expr_ta = type_check_ast(symbol_table, ast, ast[index].child1, return_type_index);

		if (!can_assign(variable_ta, expr_ta))
			log_error("Assignment to incompatible type");
		
		if (ast[index].next.has_value())
			type_check_ast(symbol_table, ast, ast[index].next.value(), return_type_index);

		return invalid_type_annotation;
	}
	else if (ast[index].type == AstNodeType::Return)
	{
		auto expr_ta = type_check_ast(symbol_table, ast, ast[index].child0, return_type_index);

		TypeAnnotation return_ta;
		return_ta.type_index = return_type_index;
		return_ta.special = false;

		if (!can_assign(return_ta, expr_ta))
			log_error("Mismatch with function return type");

		return invalid_type_annotation;
	}
	else if (ast[index].type == AstNodeType::ExpressionStatement)
	{
		type_check_ast(symbol_table, ast, ast[index].child0, return_type_index);

		if (ast[index].next.has_value())
			type_check_ast(symbol_table, ast, ast[index].next.value(), return_type_index);

		return invalid_type_annotation;
	}
	else if (ast[index].type == AstNodeType::FunctionDefinition)
	{
		type_check_ast(symbol_table, ast, ast[index].next.value(), return_type_index);

		return invalid_type_annotation;
	}
	else if (ast[index].type == AstNodeType::FunctionCall)
	{
		auto& func = symbol_table.functions[ast[index].data_function_call.function_index];
		auto& func_scope = symbol_table.scopes[func.scope];

		if (func.parameters.size() != 0)
		{
			auto current_arg_node = ast[index].child0;

			for (int i = 0; i < func.parameters.size(); i++)
			{
				auto expr_ta = type_check_ast(symbol_table, ast, current_arg_node, return_type_index);

				TypeAnnotation variable_ta;
				variable_ta.type_index = func_scope.local_variables[func.parameters[i]].type_index;
				variable_ta.special = false;

				if (!can_assign(variable_ta, expr_ta))
					log_error("Argument has incompatible type");
				
				current_arg_node = ast[current_arg_node].next.value_or(current_arg_node);
			}
		}
		
		TypeAnnotation ta;
		ta.type_index = func.return_type_index;
		ta.special = false;
		return ta;
	}
	else if (ast[index].type == AstNodeType::FunctionCallArg)
	{
		return type_check_ast(symbol_table, ast, ast[index].child0, return_type_index);
	}
	else if (ast[index].type == AstNodeType::If)
	{
		auto cond_ta = type_check_ast(symbol_table, ast, ast[index].child0, return_type_index);
		auto body_ta = type_check_ast(symbol_table, ast, ast[index].child1, return_type_index);

		if (ast[index].aux.has_value())
			auto else_ta = type_check_ast(symbol_table, ast, ast[index].aux.value(), return_type_index);

		TypeAnnotation bool_ta;
		bool_ta.special = false;
		bool_ta.type_index = intrinsic_type_index_bool;

		if (!can_assign(bool_ta, cond_ta))
			log_error("Condition doesn't match type bool");

		if (ast[index].next.has_value())
			type_check_ast(symbol_table, ast, ast[index].next.value(), return_type_index);

		return invalid_type_annotation;
	}
	else if (ast[index].type == AstNodeType::While)
	{
		auto cond_ta = type_check_ast(symbol_table, ast, ast[index].child0, return_type_index);
		auto body_ta = type_check_ast(symbol_table, ast, ast[index].child1, return_type_index);

		TypeAnnotation bool_ta;
		bool_ta.special = false;
		bool_ta.type_index = intrinsic_type_index_bool;

		if (!can_assign(bool_ta, cond_ta))
			log_error("Condition doesn't match type bool");

		if (ast[index].next.has_value())
			type_check_ast(symbol_table, ast, ast[index].next.value(), return_type_index);

		return invalid_type_annotation;
	}
	else if (ast[index].type == AstNodeType::For)
	{
		auto init_node = ast[index].child0;
		auto cond_node = ast[init_node].aux.value();
		auto incr_node = ast[cond_node].aux.value();
		auto body_node = ast[index].child1;

		auto init_ta = type_check_ast(symbol_table, ast, init_node, return_type_index);
		auto cond_ta = type_check_ast(symbol_table, ast, cond_node, return_type_index);
		auto incr_ta = type_check_ast(symbol_table, ast, incr_node, return_type_index);
		auto body_ta = type_check_ast(symbol_table, ast, body_node, return_type_index);

		TypeAnnotation bool_ta;
		bool_ta.special = false;
		bool_ta.type_index = intrinsic_type_index_bool;

		if (!can_assign(bool_ta, cond_ta))
			log_error("Condition doesn't match type bool");

		if (ast[index].next.has_value())
			type_check_ast(symbol_table, ast, ast[index].next.value(), return_type_index);

		return invalid_type_annotation;
	}
	else
	{
		internal_error("Unhandled AST node type in type_check");
	}
}

void type_check(SymbolTable& symbol_table)
{
	intrinsic_type_index_int = symbol_table.find_type("int").value();
	intrinsic_type_index_bool = symbol_table.find_type("bool").value();

	for (auto& func : symbol_table.functions)
	{
		if (func.intrinsic) continue;
		
		type_check_ast(symbol_table, func.ast, func.ast_node_root, func.return_type_index);
	}
}