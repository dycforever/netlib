
#ifndef __TOKENIZER_H__
#define __TOKENIZER_H__

#include <vector>
#include <iostream>
#include <algorithm>
#include <assert.h>

bool isBlankChar(char c);

std::string trim(const std::string& input);

size_t findStr(const std::string& input, const std::string& delimiter, size_t start=0);

void getTokens(const std::string& input, std::vector<std::string>& results, const std::string& delimiter = " ");   

size_t getToken(const std::string& input, size_t start, std::string& token, const std::string& delimiter = " ");

#endif
