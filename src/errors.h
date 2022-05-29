#pragma once

#include "lexer.h"
#include "ast.h"

#include <string>

[[noreturn]] void internal_error(const char* message);
[[noreturn]] void log_general_error(const char* message);
[[noreturn]] void log_error(const Token& token, const char* message);
[[noreturn]] void log_error(const AstNode& node, const char* message);
void log_note_type(const AstNode& node, SymbolTable& symbol_table, const char* label);
void log_note_type(const TypeAnnotation& ta, SymbolTable& symbol_table, const char* label);

void add_file_to_delete_at_exit(const std::string& file);
void delete_exit_files();