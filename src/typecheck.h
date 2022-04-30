#pragma once

#include "ast.h"

bool is_float_type(TypeAnnotation& ta);

void type_check(SymbolTable& symbol_table);