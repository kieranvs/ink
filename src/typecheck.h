#pragma once

#include "ast.h"

bool is_float_type(TypeAnnotation& ta);
bool is_float_32_type(TypeAnnotation& ta);
bool is_float_64_type(TypeAnnotation& ta);

void type_check(SymbolTable& symbol_table);