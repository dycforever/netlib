#include <vector>
#include <iostream>
#include <assert.h>

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
    return input.substr(input.find_first_not_of(" \t\n\r"), input.find_last_not_of(" \t\n\r")+1);
}

size_t findStr(const std::string& input, const std::string& delimiter, size_t start=0) {
    using namespace std;
    size_t pos = input.find(delimiter[0], start);
    size_t end = pos+delimiter.size();
    if (pos != string::npos && input.substr(pos, delimiter.size()) == delimiter) {
        return pos;
    }
    return string::npos;
}

void getToken(const std::string& input, std::vector<std::string>& results, const std::string& delimiter = " ") {
    using namespace std;
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
        if (target != string("")) {
            results.push_back(target);
        }
        start = pos + 1;
    } while (!finish);
}   

// using namespace std;
// 
// int test() {
//     vector<string> res;
//     getToken("host: localhost \r\n content-length: 123", res, "\r\n");
//     assert(res.size() == 2);
//     assert(res[0] == "host: localhost");
//     assert(res[1] == "content-length: 123");
// 
//     vector<string> res1;
//     getToken(res[0], res1, ":");
//     assert(res1.size() == 2);
//     assert(res1[0] == "host");
//     assert(res1[1] == "localhost");
// 
//     vector<string> res2;
//     getToken(res[1], res2, ":");
//     assert(res2.size() == 2);
//     assert(res2[0] == "content-length");
//     assert(res2[1] == "123");
// }
// 
