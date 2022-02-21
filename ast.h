#pragma once

#include <cstddef>

#include <vector>

enum class AstNodeType
{
	None,
	LiteralInt,
	BinOpAdd,
	BinOpMul,
	Variable,
	Assignment
};

struct AstNode
{
	AstNodeType type = AstNodeType::None;

	size_t child0;
	size_t child1;
	int data_int;
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

void dump_ast(Ast& ast, size_t index = 0, int indent = 0);