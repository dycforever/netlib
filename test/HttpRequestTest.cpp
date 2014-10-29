
#include <iostream>
#include <assert.h>
#include "HttpRequest.h"

using namespace std;
using namespace dyc;

int main () {
    HttpRequest req;
    req.setHeader("host", "localhost");
    assert(req.toString() == "GET / HTTP/1.1\nhost: localhost\n\n");
}