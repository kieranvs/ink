#pragma once

#include "lexer.h"

#include <string>
#include <vector>

struct FileData
{
	std::string name;
	std::string contents;
	std::vector<Token> tokens;
};

using FileTable = std::vector<FileData>;

extern FileTable file_table;