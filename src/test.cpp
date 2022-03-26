#include <cstdlib>
#include <cstdio>
#include <cstring>

#include <fstream>
#include <iostream>
#include <stdexcept>
#include <stdio.h>
#include <string>
#include <vector>
#include <filesystem>

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
	std::string source_file;
	int expected_error;
	std::string expected_output;
};

int main()
{
	std::vector<TestData> tests;
	auto add_test = [&tests](const std::string& src, int error, const std::string& output = "")
	{
		tests.push_back({ src, error, output });
	};

	std::string keyword = "@test ";
	for (auto const& dir_entry : std::filesystem::recursive_directory_iterator("../tests"))
	{
		if (!dir_entry.is_regular_file()) continue;

		std::ifstream input_file;
		input_file.open(dir_entry.path());
		if (input_file.fail())
		{
			printf("Couldn't open %s\n", dir_entry.path().c_str());
			continue;
		}

		std::stringstream sstr;
		sstr << input_file.rdbuf();
		std::string str = sstr.str();

		auto index = str.find(keyword);
		if (index == std::string::npos)
		{
			printf("Missing test information in %s\n", dir_entry.path().c_str());
			continue;
		}

		auto end_line = str.find("\n", index);
		if (end_line == std::string::npos)
		{
			printf("Error parsing test information in %s\n", dir_entry.path().c_str());
			continue;
		}

		auto line_str = str.substr(index + keyword.length(), end_line - index - keyword.length());
		if (line_str == "error")
		{
			add_test(dir_entry.path(), 101);
		}
		else if (line_str == "multiline")
		{
			std::string expected_output;
			auto cursor = end_line + 1;
			while (true)
			{
				if (str[cursor] == '/' && str[cursor + 1] == '/')
				{
					if (str[cursor + 2] != ' ')
						printf("%s: multiline test output comment should have initial space\n", dir_entry.path().c_str());

					auto end_line = str.find("\n", cursor);
					if (end_line == std::string::npos)
					{
						printf("Error parsing test information in %s\n", dir_entry.path().c_str());
						continue;
					}

					auto line_str = str.substr(cursor + 3, end_line - cursor - 2);

					expected_output += line_str;

					cursor = end_line + 1;
				}
				else
					break;
			}

			add_test(dir_entry.path(), 0, expected_output);
		}
		else
		{
			std::string expected_output = line_str + "\n";
			add_test(dir_entry.path(), 0, expected_output);
		}
	}

	auto num_tests = tests.size();
	size_t num_pass = 0;
	std::vector<int> failures;

	for (int i = 0; i < num_tests; i++)
	{
		printf("[%d/%d] ", i + 1, num_tests);
		bool pass = run_test(tests[i].source_file.c_str(), tests[i].expected_error, tests[i].expected_output.c_str());
		if (pass)
			num_pass += 1;
		else
			failures.push_back(i);
	}

	printf("%s", num_pass == num_tests ? CONSOLE_GRN : CONSOLE_RED);
	printf("%d/%d tests passed.\n", num_pass, num_tests);
	printf("%s", CONSOLE_NRM);

	if (!failures.empty())
	{
		printf("\nFailures:\n");
		for (auto& i : failures)
			printf("%s\n", tests[i].source_file.c_str());
	}

	return 0;
}
