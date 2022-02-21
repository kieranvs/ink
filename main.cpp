#include <stdio.h>
#include <stdlib.h>
#include <cctype>

#include <vector>
#include <optional>
#include <stack>
#include <string>
#include <fstream>
#include <sstream>

void fail(const char* message)
{
	printf(message);
	exit(-1);
}

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

void dump_ast(Ast& ast, size_t index = 0, int indent = 0)
{
	for (int i = 0; i < indent; i++)
		printf("  ");

	if (ast[index].type == AstNodeType::None)
		return;
	else if (ast[index].type == AstNodeType::LiteralInt)
		printf("%d\n", ast[index].data_int);
	else if (ast[index].type == AstNodeType::BinOpAdd)
	{
		printf("+\n");
		dump_ast(ast, ast[index].child0, indent + 1);
		dump_ast(ast, ast[index].child1, indent + 1);
	}
	else if (ast[index].type == AstNodeType::BinOpMul)
	{
		printf("*\n");
		dump_ast(ast, ast[index].child0, indent + 1);
		dump_ast(ast, ast[index].child1, indent + 1);
	}
	else if (ast[index].type == AstNodeType::Assignment)
	{
		printf("=\n");
		dump_ast(ast, ast[index].child0, indent + 1);
		dump_ast(ast, ast[index].child1, indent + 1);
	}
	else if (ast[index].type == AstNodeType::Variable)
	{
		printf("Variable\n");
	}
	else
	{
		fail("Unhandled AST node type in dump_ast\n");
	}
}

void codegen(Ast& ast, FILE* file, size_t index = 0)
{
	if (ast[index].type == AstNodeType::None)
		return;
	else if (ast[index].type == AstNodeType::LiteralInt)
		fprintf(file, "    mov rax, %d\n", ast[index].data_int);
	else if (ast[index].type == AstNodeType::BinOpAdd)
	{
		codegen(ast, file, ast[index].child0);
		fprintf(file, "    push rax\n");
		codegen(ast, file, ast[index].child1);
		fprintf(file, "    pop rbx\n");
		fprintf(file, "    add rax, rbx\n");
	}
	else if (ast[index].type == AstNodeType::BinOpMul)
	{
		codegen(ast, file, ast[index].child0);
		fprintf(file, "    push rax\n");
		codegen(ast, file, ast[index].child1);
		fprintf(file, "    pop rbx\n");
		fprintf(file, "    mul rbx\n");
	}
	else if (ast[index].type == AstNodeType::Variable)
	{
		
	}
	else if (ast[index].type == AstNodeType::Assignment)
	{
		codegen(ast, file, ast[index].child1);
	}
	else
	{
		fail("Unhandled AST node type in code gen\n");
	}
}

enum class TokenType
{
	IntegerLiteral,
	Plus,
	Multiply,
	Identifier,
	Assign
};

struct Token
{
	TokenType type;
	int data_int;
	std::string data_str;
};

struct Parser
{
	Parser(const std::vector<Token>& i) : input(i) {}

	const Token& get()
	{
		if (index >= input.size())
		{
			fail("Parser read past end of input data\n");
		}

		const Token& ret = input[index];
		index += 1;
		return ret;
	}

	bool has_more() const {
		return input.size() > index;
	}

	const std::vector<Token>& input;
	size_t index = 0;
};

size_t parse_expression(Parser& parser, Ast& ast)
{
	std::stack<size_t> expr_nodes;
	std::stack<Token> operators;
	size_t index = 0;

	auto priority = [](const Token& t)
	{
		if (t.type == TokenType::Plus)
			return 1;
		else if (t.type == TokenType::Multiply)
			return 2;
		else return 0;
	};

	auto apply_op = [&operators, &expr_nodes, &ast]()
	{
		auto& top = operators.top();
		
		size_t expr0 = expr_nodes.top();
		expr_nodes.pop();
		size_t expr1 = expr_nodes.top();
		expr_nodes.pop();

		size_t node;
		if (top.type == TokenType::Plus)
			node = ast.make(AstNodeType::BinOpAdd);
		else
			node = ast.make(AstNodeType::BinOpMul);

		ast[node].child0 = expr0;
		ast[node].child1 = expr1;

		expr_nodes.push(node);
		operators.pop();
	};

	while (parser.has_more())
	{
		auto& next_token = parser.get();
		if (next_token.type == TokenType::IntegerLiteral)
		{
			auto node = ast.make(AstNodeType::LiteralInt);
			ast[node].data_int = next_token.data_int;
			expr_nodes.push(node);
		}
		else if (next_token.type == TokenType::Plus || next_token.type == TokenType::Multiply)
		{
			while (!operators.empty() && priority(operators.top()) > priority(next_token)) // or they are the same and next_token is left assoc
			{
				apply_op();
			}

			operators.push(next_token);
		}
		else
		{
			fail("Unexpected token in expression\n");
		}
	}

	while (!operators.empty())
		apply_op();

	return expr_nodes.top();
}

size_t parse_statement(Parser& parser, Ast& ast)
{
	auto& ident_token = parser.get();
	if (ident_token.type == TokenType::Identifier)
	{
		auto& assign_token = parser.get();
		if (assign_token.type != TokenType::Assign) fail("Invalid statement\n");

		auto expr_node = parse_expression(parser, ast);
		size_t var_node = ast.make(AstNodeType::Variable);

		size_t assign_node = ast.make(AstNodeType::Assignment);
		ast[assign_node].child0 = var_node;
		ast[assign_node].child1 = expr_node;

		return assign_node;
	}
	else
	{
		fail("Invalid statement\n");
	}
}

struct Lexer
{
	Lexer(const std::string& i) : input(i) {}

	char peek()
	{
		if (index >= input.size())
		{
			fail("Lexer read past end of input data\n");
		}

		return input[index];
	}

	char get()
	{
		if (index >= input.size())
		{
			fail("Lexer read past end of input data\n");
		}

		char ret = input[index];
		index += 1;
		return ret;
	}

	template <typename FuncType>
	std::string get_while(FuncType condition)
	{
		auto start_index = index;
		while (condition(input[index])) index += 1;
		return input.substr(start_index, index - start_index);
	}

	bool get_if(char c)
	{
		if (input[index] == c)
		{
			index += 1;
			return true;
		}
		else
			return false;
	}

	bool has_more() const {
		return input.size() > index;
	}

	const std::string& input;
	size_t index = 0;
};

void lex(std::vector<Token>& tokens, Lexer& lexer)
{
	while (lexer.has_more())
	{
		if (std::isdigit(lexer.peek()))
		{
			auto literal_string = lexer.get_while([](char c) { return std::isdigit(c); });

			int x = std::stoi(literal_string);
			tokens.emplace_back();
			tokens.back().type = TokenType::IntegerLiteral;
			tokens.back().data_int = x;
		}
		else if (lexer.get_if('*'))
		{
			tokens.emplace_back();
			tokens.back().type = TokenType::Multiply;
		}
		else if (lexer.get_if('+'))
		{
			tokens.emplace_back();
			tokens.back().type = TokenType::Plus;
		}
		else if (std::isalpha(lexer.peek()))
		{
			auto identifier_string = lexer.get_while([](char c) { return std::isalnum(c); });

			tokens.emplace_back();
			tokens.back().type = TokenType::Identifier;
			tokens.back().data_str = identifier_string;
		}
		else if (lexer.get_if('='))
		{
			tokens.emplace_back();
			tokens.back().type = TokenType::Assign;
		}
		else
			lexer.get();
	}
}

int main(int argc, char** argv)
{
	std::ifstream input_file;
    input_file.open(argv[1]);

    std::stringstream sstr;
    sstr << input_file.rdbuf();
    std::string str = sstr.str();

	std::vector<Token> tokens;
	Lexer lexer(str);
	lex(tokens, lexer);

	Parser parser(tokens);

	Ast t;
	size_t st = parse_statement(parser, t);
	dump_ast(t, st);

	FILE* file_ptr = fopen("test.asm","w");
	if (file_ptr == nullptr)
		fail("Cannot open test.asm for writing!\n");

	fprintf(file_ptr, "    global    _start\n");
	fprintf(file_ptr, "\n");
	fprintf(file_ptr, "    section   .text\n");
	fprintf(file_ptr, "\n");
	fprintf(file_ptr, "_start:\n");

	codegen(t, file_ptr, st);

	fprintf(file_ptr, "    mov rdi, rax\n");
	fprintf(file_ptr, "    call print_uint32\n");
	fprintf(file_ptr, "    call exit\n");
	fprintf(file_ptr, "\n");
	fprintf(file_ptr, "exit:\n");
	fprintf(file_ptr, "    mov rax, 60\n");
	fprintf(file_ptr, "    xor rdi, rdi\n");
	fprintf(file_ptr, "    syscall\n");
	fprintf(file_ptr, "\n");
	fprintf(file_ptr, "print_uint32:\n");
	fprintf(file_ptr, "    mov eax, edi\n");
	fprintf(file_ptr, "    mov ecx, 10\n");
	fprintf(file_ptr, "    push rcx\n");
	fprintf(file_ptr, "    mov rsi, rsp\n");
	fprintf(file_ptr, "    sub rsp, 16\n");
	fprintf(file_ptr, ".toascii_digit:\n");
	fprintf(file_ptr, "    xor edx, edx\n");
	fprintf(file_ptr, "    div ecx\n");
	fprintf(file_ptr, "    add edx, '0'\n");
	fprintf(file_ptr, "    dec rsi\n");
	fprintf(file_ptr, "    mov [rsi], dl\n");
	fprintf(file_ptr, "    test eax, eax\n");
	fprintf(file_ptr, "    jnz .toascii_digit\n");
	fprintf(file_ptr, "    mov eax, 1\n");
	fprintf(file_ptr, "    mov edi, 1\n");
	fprintf(file_ptr, "    lea edx, [rsp+16 + 1]\n");
	fprintf(file_ptr, "    sub edx, esi\n");
	fprintf(file_ptr, "    syscall\n");
	fprintf(file_ptr, "    add rsp, 24\n");
	fprintf(file_ptr, "    ret\n");
	
	fclose(file_ptr);
}
