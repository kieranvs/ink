#include <cstdlib>
#include <cstdio>
#include <cstring>

#include <iostream>
#include <stdexcept>
#include <stdio.h>
#include <string>
#include <vector>

int exec_process(const char* cmd, std::string& output)
{
    char buffer[128];
    
    FILE* pipe = popen(cmd, "r");
    if (!pipe) throw std::runtime_error("popen() failed!");
    try
    {
        while (fgets(buffer, sizeof buffer, pipe) != NULL)
        	output += buffer;
    }
    catch (...)
    {
        pclose(pipe);
        throw;
    }
    
    int retval = pclose(pipe);
	if (WIFEXITED(retval) != 0)
		return WEXITSTATUS(retval);
	else
		return -1;
}

#define CONSOLE_NRM  "\x1B[0m"
#define CONSOLE_RED  "\x1B[31m"
#define CONSOLE_GRN  "\x1B[32m"
#define CONSOLE_YEL  "\x1B[33m"
#define CONSOLE_BLU  "\x1B[34m"
#define CONSOLE_MAG  "\x1B[35m"
#define CONSOLE_CYN  "\x1B[36m"
#define CONSOLE_WHT  "\x1B[37m"

bool run_test(const char* input_file, int expected_error, const char* expected_output)
{
	printf("%s... ", input_file);

	std::string compiler_command = std::string("./compiler ") + input_file;
	std::string compiler_output;
	int compiler_error = exec_process(compiler_command.c_str(), compiler_output);
	if (compiler_error != expected_error)
	{
		printf("%sFailed!%s\n", CONSOLE_RED, CONSOLE_NRM);
		printf("Compiler returned %d instead of expected %d\n", compiler_error, expected_error);
		printf("Compiler output:\n%s\n", compiler_output.c_str());
		return false;
	}

	// Compile failed, which was expected
	if (compiler_error != 0)
	{
		printf("%sPassed\n%s", CONSOLE_GRN, CONSOLE_NRM);
		return true;
	}
	
	std::string assembler_output;
	int assembler_error = exec_process("yasm -f elf64 test.asm", assembler_output);
	if (assembler_error != 0)
	{
		printf("%sFailed!%s\n", CONSOLE_RED, CONSOLE_NRM);
		printf("Assembler returned %d\n", assembler_error);
		printf("Assembler output:\n%s\n", assembler_output.c_str());
		return false;
	}
	
	std::string linker_output;
	int linker_error = exec_process("ld -o test test.o", linker_output);
	if (linker_error != 0)
	{
		printf("%sFailed!%s\n", CONSOLE_RED, CONSOLE_NRM);
		printf("Linker returned %d\n", linker_error);
		printf("Linker output:\n%s\n", linker_output.c_str());
		return false;
	}
	
	std::string runtime_output;
	int runtime_error = exec_process("./test", runtime_output);
	if (runtime_error != 0)
	{
		printf("%sFailed!%s\n", CONSOLE_RED, CONSOLE_NRM);
		printf("Program returned %d\n", runtime_error);
		printf("Program output:\n%s\n", runtime_output.c_str());
		return false;
	}
	
	if (strcmp(runtime_output.c_str(), expected_output) != 0)
	{
		printf("%sFailed!%s\n", CONSOLE_RED, CONSOLE_NRM);
		printf("Program output:\n%s\n", runtime_output.c_str());
		return false;
	}
	
	printf("%sPassed\n%s", CONSOLE_GRN, CONSOLE_NRM);
	return true;
}

struct TestData
{
	const char* source_file;
	int expected_error;
	const char* expected_output;
};

int main()
{
	std::vector<TestData> tests;
	auto add_test = [&tests](const char* src, int error, const char* output="")
	{
		tests.push_back({ src, error, output });
	};

	add_test("../tests/operator-precedence/1", 0, "23\n");
	add_test("../tests/operator-precedence/2", 0, "17\n");
	add_test("../tests/operator-precedence/3", 0, "42\n");
	add_test("../tests/operator-precedence/4", 0, "29\n");

	add_test("../tests/local-variables/1", 0, "8\n");
	add_test("../tests/local-variables/2", 0, "8\n");
	add_test("../tests/local-variables/3", 0, "9\n");
	add_test("../tests/local-variables/4", 0, "6\n");

	add_test("../tests/functions/declaration-1", 0, "1\n");
	add_test("../tests/functions/declaration-2", 0, "2\n");
	add_test("../tests/functions/call-1", 0, "32\n");
	add_test("../tests/functions/call-2", 0, "64\n");
	add_test("../tests/functions/call-3", 0, "32\n");
	add_test("../tests/functions/call-4", 0, "48\n");
	add_test("../tests/functions/call-5", 0, "64\n");

	add_test("../tests/errors-syntax/incomplete-expression-1", 101);
	add_test("../tests/errors-syntax/incomplete-expression-2", 101);
	add_test("../tests/errors-syntax/incomplete-expression-3", 101);
	add_test("../tests/errors-syntax/invalid-expression", 101);
	add_test("../tests/errors-syntax/invalid-function-declaration", 101);
	add_test("../tests/errors-syntax/unrecognised-token", 101);
	add_test("../tests/errors-syntax/assignment-to-literal", 101);
	add_test("../tests/errors-syntax/missing-function-arguments", 101);

	add_test("../tests/errors/no-main-function", 101);
	add_test("../tests/errors/undefined-variable-1", 101);
	add_test("../tests/errors/undefined-variable-2", 101);
	add_test("../tests/errors/variable-use-in-defining-statement", 101);
	add_test("../tests/errors/redefined-variable", 101);
	add_test("../tests/errors/redefined-function", 101);

	auto num_tests = tests.size();
	size_t num_pass = 0;
	for (int i = 0; i < num_tests; i++)
	{
		printf("[%d/%d] ", i + 1, num_tests);
		bool pass = run_test(tests[i].source_file, tests[i].expected_error, tests[i].expected_output);
		if (pass)
			num_pass += 1;
	}

	printf("%s", num_pass == num_tests ? CONSOLE_GRN : CONSOLE_RED);
	printf("%d/%d tests passed.\n", num_pass, num_tests);
	printf("%s", CONSOLE_NRM);

	return 0;
}
