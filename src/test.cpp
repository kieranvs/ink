#include "utils.h"

#include <cstdlib>
#include <cstdio>
#include <cstring>

#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <stdio.h>
#include <string>
#include <vector>
#include <filesystem>

#define CONSOLE_NRM  "\x1B[0m"
#define CONSOLE_RED  "\x1B[31m"
#define CONSOLE_GRN  "\x1B[32m"
#define CONSOLE_YEL  "\x1B[33m"
#define CONSOLE_BLU  "\x1B[34m"
#define CONSOLE_MAG  "\x1B[35m"
#define CONSOLE_CYN  "\x1B[36m"
#define CONSOLE_WHT  "\x1B[37m"

bool quiet = true;

bool run_test(const char* input_file, int expected_error, const char* expected_output)
{
	if (!quiet) printf("%s... ", input_file);
	const char* executable_name = std::tmpnam(nullptr);

	bool success = [&]()
	{
		char compiler_command[1024];
		snprintf(compiler_command, 1024, "./compiler %s -o %s", input_file, executable_name);
		std::string compiler_output;
		int compiler_error = exec_process(compiler_command, compiler_output);
		if (compiler_error != expected_error)
		{
			if (quiet) printf("%s... ", input_file);
			printf("%sFailed!%s\n", CONSOLE_RED, CONSOLE_NRM);
			printf("Compiler returned %d instead of expected %d\n", compiler_error, expected_error);
			if (!compiler_output.empty()) printf("Compiler output:\n%s", compiler_output.c_str());
			printf("\n");
			return false;
		}

		// Compile failed, which was expected
		if (compiler_error != 0)
		{
			if (!quiet) printf("%sPassed\n%s", CONSOLE_GRN, CONSOLE_NRM);
			return true;
		}

		std::string runtime_output;
		char runtime_command[128];
		snprintf(runtime_command, 128, "%s", executable_name);
		int runtime_error = exec_process(runtime_command, runtime_output);
		if (runtime_error != 0)
		{
			if (quiet) printf("%s... ", input_file);
			printf("%sFailed!%s\n", CONSOLE_RED, CONSOLE_NRM);
			printf("Program returned %d\n", runtime_error);
			printf("Program output:\n%s\n", runtime_output.c_str());
			return false;
		}

		if (strcmp(runtime_output.c_str(), expected_output) != 0)
		{
			if (quiet) printf("%s... ", input_file);
			printf("%sFailed!%s\n", CONSOLE_RED, CONSOLE_NRM);
			printf("Program output:\n%s\n", runtime_output.c_str());
			return false;
		}

		if (!quiet) printf("%sPassed\n%s", CONSOLE_GRN, CONSOLE_NRM);

		return true;
	}();

	remove(executable_name);

	return success;
}

struct TestData
{
	std::string source_file;
	int expected_error;
	std::string expected_output;
};

int main(int argc, const char** argv)
{
	if (argc > 1)
	{
		if (strcmp(argv[1], "-v") == 0)
			quiet = false;
		else
		{
			printf("Unrecognised option\n");
			exit(1);
		}
	}

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

	size_t num_tests = tests.size();
	size_t num_pass = 0;
	std::vector<int> failures;

	for (size_t i = 0; i < num_tests; i++)
	{
		if (!quiet) printf("[%zu/%zu] ", i + 1, num_tests);
		bool pass = run_test(tests[i].source_file.c_str(), tests[i].expected_error, tests[i].expected_output.c_str());
		if (pass)
			num_pass += 1;
		else
			failures.push_back(i);
	}

	printf("%s", num_pass == num_tests ? CONSOLE_GRN : CONSOLE_RED);
	printf("%zu/%zu tests passed.\n", num_pass, num_tests);
	printf("%s", CONSOLE_NRM);

	if (!quiet && !failures.empty())
	{
		printf("\nFailures:\n");
		for (auto& i : failures)
			printf("%s\n", tests[i].source_file.c_str());
	}

	return 0;
}
