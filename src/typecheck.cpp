#include "typecheck.h"

#include "errors.h"

size_t intrinsic_type_index_int;
size_t intrinsic_type_index_bool;
size_t intrinsic_type_index_char;
size_t intrinsic_type_index_f32;
size_t intrinsic_type_index_f64;

bool special_matches(size_t special, size_t actual)
{
	if (special == TypeAnnotation::special_type_index_literal_int)
		return actual == intrinsic_type_index_int || actual == intrinsic_type_index_char;
	else if (special == TypeAnnotation::special_type_index_literal_bool)
		return actual == intrinsic_type_index_bool;
	else if (special == TypeAnnotation::special_type_index_literal_char)
		return actual == intrinsic_type_index_char;
	else if (special == TypeAnnotation::special_type_index_literal_float)
		return actual == intrinsic_type_index_f32 || actual == intrinsic_type_index_f64;

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

bool can_combine(std::optional<TypeAnnotation>& lhs, std::optional<TypeAnnotation>& rhs, TypeAnnotation& expr_ta)
{
	if (lhs->special == rhs->special)
	{
		if (lhs->type_index == rhs->type_index)
		{
			expr_ta = *lhs;
			return true;
		}
		else
			return false;
	}
	else if (lhs->special)
	{
		if (special_matches(lhs->type_index, rhs->type_index))
		{
			expr_ta = *rhs;
			return true;
		}
		else
			return false;
	}
	else if (rhs->special)
	{
		if (special_matches(rhs->type_index, lhs->type_index))
		{
			expr_ta = *lhs;
			return true;
		}
		else
			return false;
	}
	return false;
}

bool is_bool_type(TypeAnnotation& ta)
{
	if (ta.special)
		return ta.type_index == TypeAnnotation::special_type_index_literal_bool;
	else
		return ta.type_index == intrinsic_type_index_bool;
}

bool is_number_type(TypeAnnotation& ta)
{
	if (ta.special)
		return ta.type_index == TypeAnnotation::special_type_index_literal_int
			|| ta.type_index == TypeAnnotation::special_type_index_literal_char
			|| ta.type_index == TypeAnnotation::special_type_index_literal_float;
	else
		return ta.type_index == intrinsic_type_index_int
			|| ta.type_index == intrinsic_type_index_char
			|| ta.type_index == intrinsic_type_index_f32
			|| ta.type_index == intrinsic_type_index_f64;
}

bool is_float_type(TypeAnnotation& ta)
{
	if (ta.special)
		return ta.type_index == TypeAnnotation::special_type_index_literal_float;
	else
		return ta.type_index == intrinsic_type_index_f32
			|| ta.type_index == intrinsic_type_index_f64;
}

bool is_float_32_type(TypeAnnotation& ta)
{
	return !ta.special && ta.type_index == intrinsic_type_index_f32;
}

bool is_float_64_type(TypeAnnotation& ta)
{
	if (ta.special)
		return ta.type_index == TypeAnnotation::special_type_index_literal_float;
	else
		return ta.type_index == intrinsic_type_index_f64;
}

void type_check_ast(SymbolTable& symbol_table, Ast& ast, size_t index, std::optional<size_t> return_type_index)
{
	if (ast[index].type == AstNodeType::LiteralInt)
	{
		TypeAnnotation ta;
		ta.type_index = TypeAnnotation::special_type_index_literal_int;
		ta.special = true;
		ast[index].type_annotation = ta;
		return;
	}
	else if (ast[index].type == AstNodeType::LiteralFloat)
	{
		TypeAnnotation ta;
		ta.type_index = TypeAnnotation::special_type_index_literal_float;
		ta.special = true;
		ast[index].type_annotation = ta;
		return;
	}
	else if (ast[index].type == AstNodeType::LiteralBool)
	{
		TypeAnnotation ta;
		ta.type_index = TypeAnnotation::special_type_index_literal_bool;
		ta.special = true;
		ast[index].type_annotation = ta;
		return;
	}
	else if (ast[index].type == AstNodeType::LiteralChar)
	{
		TypeAnnotation ta;
		ta.type_index = TypeAnnotation::special_type_index_literal_char;
		ta.special = true;
		ast[index].type_annotation = ta;
		return;
	}
	else if (ast[index].type == AstNodeType::LiteralString)
	{
		TypeAnnotation ta;
		ta.type_index = get_type_add_pointer(symbol_table, intrinsic_type_index_char);
		ta.special = false;
		ast[index].type_annotation = ta;
		return;
	}
	else if (ast[index].type == AstNodeType::BinOpAdd
		  || ast[index].type == AstNodeType::BinOpSub
		  || ast[index].type == AstNodeType::BinOpMul
		  || ast[index].type == AstNodeType::BinOpDiv
		  )
	{
		type_check_ast(symbol_table, ast, ast[index].child0, return_type_index);
		type_check_ast(symbol_table, ast, ast[index].child1, return_type_index);

		auto& lhs_ta = ast[ast[index].child0].type_annotation;
		auto& rhs_ta = ast[ast[index].child1].type_annotation;

		if (!lhs_ta.has_value() || !is_number_type(lhs_ta.value()))
		{
			if (lhs_ta.has_value())
				log_note_type(*lhs_ta, symbol_table, "expression");
			log_error(ast[ast[index].child0], "Non number type");
		}

		if (!rhs_ta.has_value() || !is_number_type(rhs_ta.value()))
		{
			if (rhs_ta.has_value())
				log_note_type(*rhs_ta, symbol_table, "expression");
			log_error(ast[ast[index].child1], "Non number type");
		}

		TypeAnnotation expr_ta;
		if (!can_combine(lhs_ta, rhs_ta, expr_ta))
		{
			log_note_type(*lhs_ta, symbol_table, "left");
			log_note_type(*rhs_ta, symbol_table, "right");
			log_error(ast[index], "Type mismatch");
		}

		ast[index].type_annotation = expr_ta;
		return;
	}
	else if (ast[index].type == AstNodeType::BinCompGreater
		  || ast[index].type == AstNodeType::BinCompGreaterEqual
		  || ast[index].type == AstNodeType::BinCompLess
		  || ast[index].type == AstNodeType::BinCompLessEqual
		  || ast[index].type == AstNodeType::BinCompEqual
		  || ast[index].type == AstNodeType::BinCompNotEqual
		  )
	{
		type_check_ast(symbol_table, ast, ast[index].child0, return_type_index);
		type_check_ast(symbol_table, ast, ast[index].child1, return_type_index);

		auto& lhs_ta = ast[ast[index].child0].type_annotation;
		auto& rhs_ta = ast[ast[index].child1].type_annotation;

		if (ast[index].type == AstNodeType::BinCompGreater
		  || ast[index].type == AstNodeType::BinCompGreaterEqual
		  || ast[index].type == AstNodeType::BinCompLess
		  || ast[index].type == AstNodeType::BinCompLessEqual
		  )
		{
			if (!lhs_ta.has_value() || !is_number_type(lhs_ta.value()))
			{
				if (lhs_ta.has_value())
					log_note_type(*lhs_ta, symbol_table, "expression");
				log_error(ast[ast[index].child0], "Non number type");
			}

			if (!rhs_ta.has_value() || !is_number_type(rhs_ta.value()))
			{
				if (rhs_ta.has_value())
					log_note_type(*rhs_ta, symbol_table, "expression");
				log_error(ast[ast[index].child1], "Non number type");
			}
		}

		TypeAnnotation expr_ta;
		if (!can_combine(lhs_ta, rhs_ta, expr_ta))
		{
			log_note_type(*lhs_ta, symbol_table, "left");
			log_note_type(*rhs_ta, symbol_table, "right");
			log_error(ast[index], "Type mismatch");
		}

		// Comparison makes bool
		expr_ta.special = false;
		expr_ta.type_index = intrinsic_type_index_bool;

		ast[index].type_annotation = expr_ta;
		return;
	}
	else if (ast[index].type == AstNodeType::BinLogicalAnd
		  || ast[index].type == AstNodeType::BinLogicalOr
		  )
	{
		type_check_ast(symbol_table, ast, ast[index].child0, return_type_index);
		type_check_ast(symbol_table, ast, ast[index].child1, return_type_index);

		auto& lhs_ta = ast[ast[index].child0].type_annotation;
		auto& rhs_ta = ast[ast[index].child1].type_annotation;

		if (!lhs_ta.has_value() || !is_bool_type(lhs_ta.value()))
		{
			if (lhs_ta.has_value())
				log_note_type(*lhs_ta, symbol_table, "expression");
			log_error(ast[ast[index].child0], "Non bool type");
		}

		if (!rhs_ta.has_value() || !is_bool_type(rhs_ta.value()))
		{
			if (rhs_ta.has_value())
				log_note_type(*rhs_ta, symbol_table, "expression");
			log_error(ast[ast[index].child1], "Non bool type");
		}

		TypeAnnotation expr_ta;
		if (!can_combine(lhs_ta, rhs_ta, expr_ta))
		{
			log_note_type(*lhs_ta, symbol_table, "left");
			log_note_type(*rhs_ta, symbol_table, "right");
			log_error(ast[index], "Type mismatch");
		}

		ast[index].type_annotation = expr_ta;
		return;
	}
	else if (ast[index].type == AstNodeType::Variable)
	{
		auto& scope = symbol_table.scopes[ast[index].data_variable.scope_index];
		auto variable_index = ast[index].data_variable.variable_index;
		auto& type_index = scope.local_variables[variable_index].type_index;

		TypeAnnotation ta;
		ta.type_index = type_index;
		ta.special = false;
		ast[index].type_annotation = ta;
		return;
	}
	else if (ast[index].type == AstNodeType::VariableGlobal)
	{
		auto variable_index = ast[index].data_variable.variable_index;
		auto& type_index = symbol_table.global_variables[variable_index].type_index;

		TypeAnnotation ta;
		ta.type_index = type_index;
		ta.special = false;
		ast[index].type_annotation = ta;
		return;
	}
	else if (ast[index].type == AstNodeType::Selector)
	{
		auto& scope = symbol_table.scopes[ast[index].data_variable.scope_index];
		auto variable_index = ast[index].data_variable.variable_index;
		auto& type_index = scope.local_variables[variable_index].type_index;

		TypeAnnotation ta;
		ta.type_index = type_index;
		ta.special = false;
		ast[index].type_annotation = ta;
		return;
	}
	else if (ast[index].type == AstNodeType::Assignment)
	{
		type_check_ast(symbol_table, ast, ast[index].child0, return_type_index);
		type_check_ast(symbol_table, ast, ast[index].child1, return_type_index);

		auto& variable_ta = ast[ast[index].child0].type_annotation;
		auto& expr_ta = ast[ast[index].child1].type_annotation;

		if (!variable_ta.has_value() || !expr_ta.has_value() || !can_assign(variable_ta.value(), expr_ta.value()))
		{
			log_note_type(ast[ast[index].child0], symbol_table, "left");
			log_note_type(ast[ast[index].child1], symbol_table, "right");
			log_error(ast[index], "Assignment to incompatible type");
		}
		
		if (ast[index].next.has_value())
			type_check_ast(symbol_table, ast, ast[index].next.value(), return_type_index);

		return;
	}
	else if (ast[index].type == AstNodeType::ZeroInitialise)
	{
		type_check_ast(symbol_table, ast, ast[index].child0, return_type_index);

		if (ast[index].next.has_value())
			type_check_ast(symbol_table, ast, ast[index].next.value(), return_type_index);

		return;
	}
	else if (ast[index].type == AstNodeType::Return)
	{
		if (return_type_index.has_value())
		{
			if (!ast[index].aux.has_value())
				log_error(ast[index], "Missing function return value");

			type_check_ast(symbol_table, ast, ast[index].aux.value(), return_type_index);
			auto& expr_ta = ast[ast[index].aux.value()].type_annotation;

			TypeAnnotation return_ta;
			return_ta.type_index = return_type_index.value();
			return_ta.special = false;

			if (!expr_ta.has_value() || !can_assign(return_ta, expr_ta.value()))
			{
				log_note_type(ast[ast[index].aux.value()], symbol_table, "expression");
				log_note_type(return_ta, symbol_table, "function return");
				log_error(ast[ast[index].aux.value()], "Mismatch with function return type");
			}
		}
		else
		{
			if (ast[index].aux.has_value())
				log_error(ast[ast[index].aux.value()], "Function doesn't return a value");
		}

		return;
	}
	else if (ast[index].type == AstNodeType::ExpressionStatement)
	{
		type_check_ast(symbol_table, ast, ast[index].child0, return_type_index);

		if (ast[index].next.has_value())
			type_check_ast(symbol_table, ast, ast[index].next.value(), return_type_index);

		return;
	}
	else if (ast[index].type == AstNodeType::FunctionDefinition)
	{
		type_check_ast(symbol_table, ast, ast[index].next.value(), return_type_index);

		return;
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
				type_check_ast(symbol_table, ast, current_arg_node, return_type_index);
				auto& expr_ta = ast[current_arg_node].type_annotation;

				TypeAnnotation variable_ta;
				variable_ta.type_index = func_scope.local_variables[func.parameters[i]].type_index;
				variable_ta.special = false;

				if (!expr_ta.has_value() || !can_assign(variable_ta, expr_ta.value()))
				{
					log_note_type(variable_ta, symbol_table, "argument");
					if (expr_ta.has_value())
						log_note_type(*expr_ta, symbol_table, "expression");
					log_error(ast[current_arg_node], "Argument has incompatible type");
				}
				
				current_arg_node = ast[current_arg_node].next.value_or(current_arg_node);
			}
		}
		
		if (func.return_type_index.has_value())
		{
			TypeAnnotation ta;
			ta.type_index = func.return_type_index.value();
			ta.special = false;
			ast[index].type_annotation = ta;
		}
		return;
	}
	else if (ast[index].type == AstNodeType::FunctionCallArg)
	{
		type_check_ast(symbol_table, ast, ast[index].child0, return_type_index);
		ast[index].type_annotation = ast[ast[index].child0].type_annotation.value();
		return;
	}
	else if (ast[index].type == AstNodeType::If)
	{
		type_check_ast(symbol_table, ast, ast[index].child0, return_type_index);
		type_check_ast(symbol_table, ast, ast[index].child1, return_type_index);

		auto& cond_ta = ast[ast[index].child0].type_annotation;

		if (ast[index].aux.has_value())
			type_check_ast(symbol_table, ast, ast[index].aux.value(), return_type_index);

		TypeAnnotation bool_ta;
		bool_ta.special = false;
		bool_ta.type_index = intrinsic_type_index_bool;

		if (!cond_ta.has_value() || !can_assign(bool_ta, cond_ta.value()))
		{
			if (cond_ta.has_value())
				log_note_type(*cond_ta, symbol_table, "condition");
			log_error(ast[ast[index].child0], "Condition doesn't match type bool");
		}

		if (ast[index].next.has_value())
			type_check_ast(symbol_table, ast, ast[index].next.value(), return_type_index);

		return;
	}
	else if (ast[index].type == AstNodeType::While)
	{
		type_check_ast(symbol_table, ast, ast[index].child0, return_type_index);
		type_check_ast(symbol_table, ast, ast[index].child1, return_type_index);

		auto& cond_ta = ast[ast[index].child0].type_annotation;

		TypeAnnotation bool_ta;
		bool_ta.special = false;
		bool_ta.type_index = intrinsic_type_index_bool;

		if (!cond_ta.has_value() || !can_assign(bool_ta, cond_ta.value()))
		{
			if (cond_ta.has_value())
				log_note_type(*cond_ta, symbol_table, "condition");
			log_error(ast[ast[index].child0], "Condition doesn't match type bool");
		}

		if (ast[index].next.has_value())
			type_check_ast(symbol_table, ast, ast[index].next.value(), return_type_index);

		return;
	}
	else if (ast[index].type == AstNodeType::For)
	{
		auto init_node = ast[index].child0;
		auto cond_node = ast[init_node].aux.value();
		auto incr_node = ast[cond_node].aux.value();
		auto body_node = ast[index].child1;

		type_check_ast(symbol_table, ast, init_node, return_type_index);
		type_check_ast(symbol_table, ast, cond_node, return_type_index);
		type_check_ast(symbol_table, ast, incr_node, return_type_index);
		type_check_ast(symbol_table, ast, body_node, return_type_index);

		auto& cond_ta = ast[cond_node].type_annotation;

		TypeAnnotation bool_ta;
		bool_ta.special = false;
		bool_ta.type_index = intrinsic_type_index_bool;

		if (!cond_ta.has_value() || !can_assign(bool_ta, cond_ta.value()))
		{
			if (cond_ta.has_value())
				log_note_type(*cond_ta, symbol_table, "condition");
			log_error(ast[cond_node], "Condition doesn't match type bool");
		}

		if (ast[index].next.has_value())
			type_check_ast(symbol_table, ast, ast[index].next.value(), return_type_index);

		return;
	}
	else if (ast[index].type == AstNodeType::AddressOf)
	{
		auto child_node = ast[index].child0;
		if (ast[child_node].type == AstNodeType::Function)
		{
			auto& func = symbol_table.functions[ast[child_node].data_function_call.function_index];
			auto& func_scope = symbol_table.scopes[func.scope];

			std::vector<size_t> parameter_types;
			for (auto var_index : func.parameters)
			{
				auto var_type = func_scope.local_variables[var_index].type_index;
				parameter_types.push_back(var_type);
			}

			auto type_index = symbol_table.find_matching_function_type(parameter_types, func.return_type_index);
			if (type_index.has_value())
			{
				TypeAnnotation ta;
				ta.special = false;
				ta.type_index = type_index.value();
				ast[index].type_annotation = ta;
			}
			else
			{
				log_error(ast[index], "Function used in expression without matching function type");
			}
		}
		else
		{
			type_check_ast(symbol_table, ast, ast[index].child0, return_type_index);

			auto& expr_ta = ast[ast[index].child0].type_annotation;

			if (expr_ta->special)
				log_error(ast[index], "Address of literal");

			TypeAnnotation ta;
			ta.special = false;
			ta.type_index = get_type_add_pointer(symbol_table, expr_ta->type_index);
			ast[index].type_annotation = ta;
		}

		return;
	}
	else if (ast[index].type == AstNodeType::Dereference)
	{
		type_check_ast(symbol_table, ast, ast[index].child0, return_type_index);

		auto& expr_ta = ast[ast[index].child0].type_annotation;

		if (expr_ta->special)
			log_error(ast[index], "Dereference of literal");

		if (symbol_table.types[expr_ta->type_index].type != TypeType::Pointer)
		{
			log_note_type(*expr_ta, symbol_table, "expression");
			log_error(ast[index], "Dereference of non-pointer");
		}

		TypeAnnotation ta;
		ta.special = false;
		ta.type_index = get_type_remove_pointer(symbol_table, expr_ta->type_index);
		ast[index].type_annotation = ta;
		return;
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
	intrinsic_type_index_char = symbol_table.find_type("char").value();
	intrinsic_type_index_f32 = symbol_table.find_type("f32").value();
	intrinsic_type_index_f64 = symbol_table.find_type("f64").value();

	for (auto& func : symbol_table.functions)
	{
		if (func.intrinsic || func.is_external) continue;
		
		type_check_ast(symbol_table, func.ast, func.ast_node_root, func.return_type_index);
	}
}