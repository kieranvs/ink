#pragma once

#include "ast.h"

#include <stdio.h>
#include <stdlib.h>

void codegen(Ast& ast, FILE* file, size_t index = 0);