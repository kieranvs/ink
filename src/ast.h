#pragma once

#include "lexer.h"

#include <cstddef>
#include <stdint.h>
#include <stdio.h>

#include <vector>
#include <string>
#include <optional>

struct TypeAnnotation
{
	size_t type_index;
	bool special;
};

enum class AstNodeType
{
	None,
	LiteralInt,
	LiteralBool,
	LiteralChar,
	LiteralString,
	BinOpAdd,
	BinOpSub,
	BinOpMul,
	BinOpDiv,
	BinCompGreater,
	BinCompGreaterEqual,
	BinCompLess,
	BinCompLessEqual,
	BinCompEqual,
	BinCompNotEqual,
	BinLogicalAnd,
	BinLogicalOr,
	Variable,
	Assignment,
	Return,
	ExpressionStatement,
	FunctionDefinition,
	FunctionCall,
	FunctionCallArg,
	If,
	While,
	For,
	AddressOf,
	Dereference
};

struct AstNode
{
	AstNodeType type = AstNodeType::None;

	size_t child0; // LHS or only child, condition expr for if
	size_t child1; // RHS, if branch for if
	std::optional<size_t> next; // Next in block for statements
	std::optional<size_t> aux; // Else branch for if

	SourceLocation location;
	std::optional<TypeAnnotation> type_annotation;

	struct DataLiteralInt
	{
		int value;
	};

	struct DataLiteralBool
	{
		bool value;
	};

	struct DataLiteralString
	{
		size_t constant_string_index;
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
		DataLiteralString data_literal_string;
		DataVariable data_variable;
		DataFunctionDefinition data_function_definition;
		DataFunctionCall data_function_call;
	};
};

struct Ast
{
	std::vector<AstNode> nodes;
	
	size_t make(AstNodeType type, const Token& token)
	{
		size_t index = nodes.size();
		nodes.emplace_back();
		nodes[index].type = type;
		nodes[index].location = token.location;
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

	bool is_pointer = false;
	size_t remove_ptr_type;
	std::optional<size_t> add_ptr_type;
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

struct ConstantString
{
	ConstantString(const std::string& s) : str(s) {}

	std::string str;
};

struct SymbolTable
{
	std::vector<Function> functions;
	std::vector<Scope> scopes;
	std::vector<Type> types;
	std::vector<ConstantString> constant_strings;

	std::optional<std::pair<size_t, size_t>> find_variable(size_t scope_index, const std::string& name);
	std::optional<size_t> find_function(const std::string& name);
	std::optional<size_t> find_type(const std::string& name);
	size_t find_add_string(const std::string& str);
};

void dump_ast(FILE* output, SymbolTable& symbol_table, Ast& ast, size_t index = 0, int indent = 0);
size_t get_type_add_pointer(SymbolTable& symbol_table, size_t base_type);
size_t get_type_remove_pointer(SymbolTable& symbol_table, size_t ptr_type);