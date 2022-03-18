#pragma once

#include "lexer.h"

void set_current_file(const char* file_source);

void internal_error(const char* message);
void log_error(const char* message);
void log_error(const Token& token, const char* message);