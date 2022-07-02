#pragma once

#include <string>

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

void print_stack_trace();

std::string get_relative_path(const std::string& from_file, const std::string& rel_path);