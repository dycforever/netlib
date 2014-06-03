#ifndef TOKENIZER_H
#define TOKENIZER_H

#include <Tokenizer.h>

bool isBlankChar(char c) {
    if (c == ' ' 
        ||  c == '\t'
        ||  c == '\r'
        ||  c == '\n')
        return true;
    return false;
}

std::string trim(const std::string& input) {
    size_t size = input.size();
    if (size == 0 || 
            (!isBlankChar(input[0]) && !isBlankChar(input[size-1]) )
            ) {
        return input;
    }
    size_t start = input.find_first_not_of(" \t\n\r");
    size_t end = input.find_last_not_of(" \t\n\r");
    if (start >= end) {
        return "";
    }
    return input.substr(start, end+1);
}

size_t findStr(const std::string& input, const std::string& delimiter, size_t start) {
    using namespace std;
//    size_t pos = input.find(delimiter[0], start);
//    size_t end = pos+delimiter.size();
//    if (pos != string::npos && input.substr(pos, delimiter.size()) == delimiter) {
//        return pos;
//    }
//    return string::npos;
    string::const_iterator pos = search(input.begin()+start, input.end(), delimiter.begin(), delimiter.end());
    if (pos == input.end())
        return string::npos;
    else 
        return pos-input.begin();
}

void getTokens(const std::string& input, std::vector<std::string>& results, const std::string& delimiter) {
    using namespace std;
    results.clear();
    if (delimiter == "")
        return;
    size_t pos = 0;
    size_t start = 0;
    bool finish = false;
    do {
        pos = findStr(input, delimiter, start);
        if (pos == string::npos) {
            finish = true;
        }
        string target = trim(input.substr(start, pos-start));
//        if (target != string("")) {
            results.push_back(target);
//        }
        start = pos + 1;
    } while (!finish);
}   



size_t getToken(const std::string& input, size_t start, std::string& token, const std::string& delimiter) {
    using namespace std;
    if (delimiter == "")
        return input.size();
    size_t pos = 0;
    pos = findStr(input, delimiter, start);
    if (pos == string::npos) {
        token = trim(input.substr(start, input.size()-start));
        return pos;
    }
    token = trim(input.substr(start, pos-start));
    return pos + delimiter.size();
} 


#endif  // CONDITION_H
