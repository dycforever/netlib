
#include "Buffer.h"
#include <string>
#include <iostream>

using namespace std;


int main () {
    Buffer buffer;
    string s = "0123456789";
    buffer.append(s);

    assert(buffer.readableBytes() == s.size());
    for (int i=0; i<s.size(); i++) {
        char c = *(buffer.getData(1));
        assert((c) == '0'+i);
    }

    assert(buffer.readableBytes() == 0);
}
