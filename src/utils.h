#pragma once

#include "lexer.h"

#include <string>

void set_current_file(const char* file_source);

void internal_error(const char* message);
void log_error(const char* message);
void log_error(const Token& token, const char* message);

void add_file_to_delete_at_exit(const std::string& file);
void delete_exit_files();

int exec_process(const char* cmd, std::string& output);