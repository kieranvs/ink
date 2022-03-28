#pragma once

#include "lexer.h"

#include <cstddef>
#include <stdint.h>
#include <stdio.h>

#include <vector>
#include <string>
#include <optional>

enum class AstNodeType
{
	None,
	LiteralInt,
	LiteralBool,
	BinOpAdd,
	BinOpMul,
	BinCompGreater,
	BinCompGreaterEqual,
	BinCompLess,
	BinCompLessEqual,
	BinCompEqual,
	BinCompNotEqual,
	Variable,
	Assignment,
	Return,
	ExpressionStatement,
	FunctionDefinition,
	FunctionCall,
	FunctionCallArg,
	If,
	While
};

struct AstNode
{
	AstNodeType type = AstNodeType::None;

	size_t child0; // LHS or only child, condition expr for if
	size_t child1; // RHS, if branch for if
	std::optional<size_t> next; // Next in block for statements
	std::optional<size_t> aux; // Else branch for if

	struct DataLiteralInt
	{
		int value;
	};

	struct DataLiteralBool
	{
		bool value;
	};

	struct DataVariable
	{
		size_t scope_index;
		size_t variable_index;
	};

	struct DataFunctionDefinition
	{
		size_t function_index;
		int stack_size;
	};

	struct DataFunctionCall
	{
		size_t function_index;
	};

	union
	{
		DataLiteralInt data_literal_int;
		DataLiteralBool data_literal_bool;
		DataVariable data_variable;
		DataFunctionDefinition data_function_definition;
		DataFunctionCall data_function_call;
	};
};

struct Ast
{
	std::vector<AstNode> nodes;
	
	size_t make(AstNodeType type)
	{
		size_t index = nodes.size();
		nodes.emplace_back();
		nodes[index].type = type;
		return index;
	}

	AstNode& operator[](size_t index) { return nodes[index]; }
};

struct Variable
{
	uint32_t scope_offset;
	uint32_t stack_offset;
	std::string name;
	size_t type_index;
};

struct Type
{
	std::string name;
	bool intrinsic;
	size_t data_size;
};

struct SymbolTable;

struct Scope
{
	std::vector<Variable> local_variables;
	std::optional<size_t> parent;

	std::optional<size_t> make_variable(SymbolTable& symbol_table, const std::string& name, size_t type_index);
};

struct Function
{
	size_t scope;
	Ast ast;
	size_t ast_node_root;
	std::string name;
	std::vector<size_t> parameters;
	bool intrinsic;
	size_t return_type_index;
	size_t next_label = 0;
};

struct SymbolTable
{
	std::vector<Function> functions;
	std::vector<Scope> scopes;
	std::vector<Type> types;

	std::optional<std::pair<size_t, size_t>> find_variable(size_t scope_index, const std::string& name);
	std::optional<size_t> find_function(const std::string& name);
	std::optional<size_t> find_type(const std::string& name);
};

void dump_ast(FILE* output, SymbolTable& symbol_table, Ast& ast, size_t index = 0, int indent = 0);