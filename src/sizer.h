#pragma once

#include "ast.h"

void compute_sizing(SymbolTable& symbol_table);

size_t get_data_size(SymbolTable& symbol_table, const TypeAnnotation& type_annotation);