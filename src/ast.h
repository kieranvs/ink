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

	constexpr static size_t special_type_index_literal_int = 0;
	constexpr static size_t special_type_index_literal_bool = 1;
	constexpr static size_t special_type_index_literal_char = 2;
	constexpr static size_t special_type_index_literal_float = 3;

	// Indices for quick identification of intrinsic types
	static size_t intrinsic_type_index_int;
	static size_t intrinsic_type_index_bool;
	static size_t intrinsic_type_index_char;
	static size_t intrinsic_type_index_f32;
	static size_t intrinsic_type_index_f64;

	enum class ModifierType
	{
		Pointer
	};

	struct Modifier
	{
		ModifierType type;
		size_t modifier_amount;
	};

	constexpr static size_t max_modifiers = 4;
	Modifier modifiers[max_modifiers];
	uint8_t modifiers_in_use = 0;

	TypeAnnotation add_pointer() const;
	TypeAnnotation remove_pointer() const;
};

enum class AstNodeType
{
	None,
	LiteralInt,
	LiteralFloat,
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
	VariableGlobal,
	Selector,
	Assignment,
	ZeroInitialise,
	Return,
	ExpressionStatement,
	FunctionDefinition,
	FunctionCall,
	FunctionCallArg,
	Function,
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

	struct DataLiteralFloat
	{
		size_t constant_float_index;
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
		DataLiteralFloat data_literal_float;
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
	uint32_t stack_offset;
	std::string name;
	TypeAnnotation type_annotation;
};

enum class TypeType
{
	Intrinsic,
	Struct,
	Alias,
	Function,
	Incomplete
};

struct Type
{
	std::string name;
	TypeType type;
	SourceLocation location; // For printing errors etc

	size_t data_size;

	// Set for structs
	size_t scope;

	// Set for alias
	size_t actual_type;

	// Set for function types
	size_t function_type_index;
};

struct SymbolTable;

struct Scope
{
	std::vector<Variable> local_variables;
	std::optional<size_t> parent;

	std::optional<size_t> make_variable(SymbolTable& symbol_table, const std::string& name, const TypeAnnotation& type_annotation);
};

struct Function
{
	size_t scope;
	Ast ast;
	size_t ast_node_root;
	std::string name;
	std::string asm_name;
	std::vector<size_t> parameters; // Indices of the parameters in the local variables of the scope
	bool intrinsic;
	std::optional<TypeAnnotation> return_type;
	size_t next_label = 0;
	bool is_external = false;
};

struct FunctionType
{
	std::vector<TypeAnnotation> parameter_types;
	std::optional<TypeAnnotation> return_type_index;
};

struct ConstantString
{
	ConstantString(const std::string& s) : str(s) {}

	std::string str;
};

struct LinkerPath
{
	LinkerPath(const std::string& p, bool is_framework) : path(p), is_macos_framework(is_framework) {}

	std::string path;
	bool is_macos_framework;
};

struct VariableFindResult
{
	bool is_global;
	size_t scope_index;
	size_t variable_index;
};

struct SymbolTable
{
	std::vector<Function> functions;
	std::vector<Scope> scopes;
	std::vector<Variable> global_variables;
	std::vector<Type> types;
	std::vector<FunctionType> function_types;
	std::vector<ConstantString> constant_strings;
	std::vector<double> constant_floats;
	std::vector<LinkerPath> linker_paths;

	std::optional<VariableFindResult> find_variable(size_t scope_index, const std::string& name);
	std::optional<size_t> find_function(const std::string& name);
	std::optional<size_t> find_type(const std::string& name);
	size_t find_add_type(const std::string& name, const Token& token);
	std::optional<size_t> find_matching_function_type(const std::vector<TypeAnnotation>& parameter_types, const std::optional<TypeAnnotation>& return_type);

	bool check_equivalent(const TypeAnnotation& a, const TypeAnnotation& b) const;

	size_t find_add_string(const std::string& str);
	size_t find_add_float(double value);

	void add_linker_path(const std::string& path, bool is_macos_framework);
};

void pretty_print_type(FILE* output, const SymbolTable& symbol_table, const TypeAnnotation& type_annotation);
void dump_symbol_table(FILE* output, SymbolTable& symbol_table);

bool is_bool_type(TypeAnnotation& ta);
bool is_number_type(TypeAnnotation& ta);
bool is_float_type(TypeAnnotation& ta);
bool is_float_32_type(TypeAnnotation& ta);
bool is_float_64_type(TypeAnnotation& ta);
bool is_struct_type(SymbolTable& symbol_table, TypeAnnotation& ta);