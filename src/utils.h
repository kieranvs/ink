#pragma once

#include "lexer.h"
#include "ast.h"

#include <string>

void set_current_file(const char* file_source);

[[noreturn]] void internal_error(const char* message);
[[noreturn]] void log_general_error(const char* message);
[[noreturn]] void log_error(const Token& token, const char* message);
[[noreturn]] void log_error(const AstNode& node, const char* message);

void add_file_to_delete_at_exit(const std::string& file);
void delete_exit_files();

int exec_process(const char* cmd, std::string& output);

enum class Platform
{
	Linux,
	MacOS
};
inline constexpr Platform get_platform()
{
#if defined(__linux__)
	return Platform::Linux;
#elif defined(__APPLE__)
	return Platform::MacOS;
#else
	#error "Unsupported platform"
#endif
}