#pragma once

#include "ast.h"

#include <stdio.h>
#include <stdlib.h>

void codegen(SymbolTable& symbol_table, FILE* file);