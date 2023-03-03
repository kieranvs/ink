#include "sizer.h"

#include "errors.h"

// Assigns offsets to each variable in the scope using the data type size.
// Recurses into the child scopes to assign their variables too.
uint32_t assign_stack_offsets(SymbolTable& symbol_table, uint32_t base_offset, size_t scope_index)
{
	auto& scope = symbol_table.scopes[scope_index];
	uint32_t scope_size = 0;
	for (size_t var_index = 0; var_index < scope.local_variables.size(); var_index++)
	{
		auto& var = scope.local_variables[var_index];
		auto data_size = get_data_size(symbol_table, var.type_annotation);
		if (var_index == 0)
			var.stack_offset = data_size + base_offset;
		else
			var.stack_offset = scope.local_variables[var_index - 1].stack_offset + data_size;
		scope_size += data_size;
	}

	// This is an inefficient way to find the child scopes
	uint32_t biggest_child_size = 0;
	for (size_t i = 0; i < symbol_table.scopes.size(); i++)
	{
		auto& child_scope = symbol_table.scopes[i];
		if (!child_scope.parent.has_value() || child_scope.parent.value() != scope_index) continue;

		uint32_t child_size = assign_stack_offsets(symbol_table, base_offset + scope_size, i);
		if (child_size > biggest_child_size)
			biggest_child_size = child_size;
	}

	return biggest_child_size + scope_size;
}

// Calls assign_stack_offsets on all the scopes this one depends on, and also on the current one.
// Keeps track of which ones are visited in order to be used as part of the topsort calculation.
void assign_stack_offsets_dependent_scopes(SymbolTable& symbol_table, size_t scope_index, std::vector<bool>& visited)
{
	// Scopes which are children of other scopes can be skipped because they are handled
	// when their parent is handled since assign_stack_offsets recurses
	if (symbol_table.scopes[scope_index].parent.has_value())
	{
		visited[scope_index] = true;
		return;
	}

	// If this scope contains structs, need to process the struct's scope first
	for (auto& var : symbol_table.scopes[scope_index].local_variables)
	{
		auto& type = symbol_table.types[var.type_annotation.type_index];
		if (is_struct_type(symbol_table, var.type_annotation)) // will break when we have arrays - need to check if it depends on the member types not being incomplete, i.e. pointers are okay
		{
			if (!visited[type.scope])
				assign_stack_offsets_dependent_scopes(symbol_table, type.scope, visited);
		}
	}

	// The same applies for any child scopes of this scope
	for (size_t child_index = 0; child_index < symbol_table.scopes.size(); child_index++)
	{
		auto& child_scope = symbol_table.scopes[child_index];

		if (!child_scope.parent.has_value() || child_scope.parent.value() != scope_index) continue;

		for (auto& var : symbol_table.scopes[child_index].local_variables)
		{
			auto& type = symbol_table.types[var.type_annotation.type_index];
			if (is_struct_type(symbol_table, var.type_annotation)) // will break when we have arrays - need to check if it depends on the member types not being incomplete, i.e. pointers are okay
			{
				if (!visited[type.scope])
					assign_stack_offsets_dependent_scopes(symbol_table, type.scope, visited);
			}
		}
	}
	visited[scope_index] = true;

	// Now actually assign_stack_offsets for this scope
	for (auto& func : symbol_table.functions)
	{
		if (func.scope == scope_index && !func.intrinsic && !func.is_external)
		{
			auto stack_size = assign_stack_offsets(symbol_table, 0, func.scope);
			if (stack_size % 16 != 0)
				stack_size = ((stack_size / 16) + 1) * 16;
			func.ast[func.ast_node_root].data_function_definition.stack_size = stack_size;

			break;
		}
	}

	for (auto& type : symbol_table.types)
	{
		if (type.type == TypeType::Struct && type.scope == scope_index)
		{
			type.data_size = assign_stack_offsets(symbol_table, 0, type.scope);
			break;
		}
	}
}

// Calls assign_stack_offsets for all scopes in the symbol table, in the order that their
// dependencies require.
void assign_stack_offsets_all_scopes(SymbolTable& symbol_table)
{
	std::vector<bool> visited(symbol_table.scopes.size());
	for (size_t unvisited = 0; unvisited < symbol_table.scopes.size(); unvisited++)
	{
		if (!visited[unvisited])
			assign_stack_offsets_dependent_scopes(symbol_table, unvisited, visited);
	}
}

void compute_sizing(SymbolTable& symbol_table)
{
	for (size_t type_index = 0; type_index < symbol_table.types.size(); type_index++)
	{
		const auto& type = symbol_table.types[type_index];
		if (type.type == TypeType::Incomplete)
			log_error(type, "Undefined type");
	}

	assign_stack_offsets_all_scopes(symbol_table);
}

size_t get_data_size(SymbolTable& symbol_table, const TypeAnnotation& type_annotation)
{
	size_t size = symbol_table.types[type_annotation.type_index].data_size;

	for (int i = 0; i < type_annotation.modifiers_in_use; i++)
	{
		if (type_annotation.modifiers[i].type == TypeAnnotation::ModifierType::Pointer)
			size = 8;
	}

	return size;
}