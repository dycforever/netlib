#include <Tokenizer.h>
#include <iostream>
#include <algorithm>
#include <stdio.h>

using namespace std;

int test() {
    vector<string> res;
    getTokens("host: localhost \r\n content-length: 123", res, "\r\n");
    assert(res.size() == 2);
    assert(res[0] == "host: localhost");
    assert(res[1] == "content-length: 123");

    vector<string> res1;
    getTokens(res[0], res1, ":");
    assert(res1.size() == 2);
    assert(res1[0] == "host");
    assert(res1[1] == "localhost");

    vector<string> res2;
    getTokens(res[1], res2, ":");
    assert(res2.size() == 2);
    assert(res2[0] == "content-length");
    assert(res2[1] == "123");
}

int main () {
    string line;
    string fs = "\r\n";
    char buf[1024];
    int count = fread(buf, 1, 849, stdin);
    cout << "read count: " << count << endl;

    string txt(buf, 849);
//     while (getline(cin, line)) {
//         txt += line;
//         txt += "\r\n";
//     }
    cout << "txt size:  " << txt.size() << endl;
    vector<string> res;
//    getToken(txt, res, "\r\n");
    string::iterator fp = search(txt.begin(), txt.end(), fs.begin(), fs.end());
    if (fp == txt.begin()) {
        cout << "not found" << endl;
    } else {
        cout << "found" << endl;
    }
//    cout << "tokens size:" << res.size() << endl;
}
