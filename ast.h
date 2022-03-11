#pragma once

#include "lexer.h"

#include <cstddef>
#include <stdint.h>

#include <vector>
#include <string>
#include <optional>

enum class AstNodeType
{
	None,
	LiteralInt,
	BinOpAdd,
	BinOpMul,
	Variable,
	Assignment,
	Return,
	FunctionDefinition,
	FunctionCall
};

struct AstNode
{
	AstNodeType type = AstNodeType::None;

	size_t child0;
	size_t child1;
	std::optional<size_t> next;

	struct DataLiteralInt
	{
		int value;
	};

	struct DataVariable
	{
		int offset;
	};

	struct DataFunctionDefinition
	{
		int stack_size;
	};

	struct DataFunctionCall
	{
		size_t function_index;
	};

	union
	{
		DataLiteralInt data_literal_int;
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
	uint32_t stack_offset;
	std::string name;
};

struct Scope
{
	std::vector<Variable> local_variables;
	std::optional<size_t> parent;

	const Variable* find_variable(const std::string& name, bool create_if_missing);
};

struct Function
{
	size_t scope;
	Ast ast;
	size_t ast_node_root;
	std::string name;
};

struct SymbolTable
{
	std::vector<Function> functions;
	std::vector<Scope> scopes;

	std::optional<size_t> find_function(const std::string& name);
};

void dump_ast(Ast& ast, size_t index = 0, int indent = 0);