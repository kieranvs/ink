#include "ast.h"
#include "lexer.h"
#include "parser.h"
#include "codegen.h"
#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <cctype>

#include <vector>
#include <optional>
#include <string>
#include <fstream>
#include <sstream>

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
