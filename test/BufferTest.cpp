
#include <Buffer.h>
#include <string>
#include <iostream>

using namespace std;


int main () {
    {
        char b[1024];
        Buffer buffer(b, 1024);
        assert(buffer.readableSize() == 1024);
    }

    {
        Buffer buffer(1024);
        string s = "0123456789";
        buffer.append(s);

        assert(buffer.readableSize() == s.size());
        for (int i = 0; i < s.size(); i++) {
            char c = *(buffer.get(1));
            assert((c) == '0'+i);
        }

        assert(buffer.readableSize() == 0);
    }

    for (int casei = 0 ; casei < 100; casei++) {
        Buffer buffer(1024);
        string s = "0123456789";
        buffer.append(s);

        assert(buffer.readableSize() == s.size());
        for (int i = 0; i < 5; i++) {
            char c = *(buffer.get(1));
            assert((c) == '0' + i);
        }
        assert(buffer.readableSize() == 5);
        buffer.append(s);
        for (int i = 0; i < 5; i++) {
            char c = *(buffer.get(1));
            assert((c) == '0' + i + 5);
        }
        for (int i = 0; i < 10; i++) {
            char c = *(buffer.get(1));
            assert((c) == '0'+i);
        }

        assert(buffer.readableSize() == 0);
    }
}
