#include <iostream>
#include <stdint.h>
#include "block_cache.h"
#include "block.h"
#include "util.h"
#include <istream>
#include <fstream>
#include <unistd.h>
using namespace std;

int
main() {
    freopen("/home/arvin/log.txt", "w", stdout);
    // create some data
    ifstream is("/home/arvin/Makefile", ifstream::binary);
    uint8_t* buf;
    if (is) {
        is.seekg(0, is.end);
        int len = is.tellg();
        is.seekg(0, is.beg);
        buf = new uint8_t[len];

        // cout << "Reading" << len << "things " << endl;
        is.read((char*)buf, len);
        if (is)
            cout << "read success" << endl;
        else
            cout << "read fail" << endl;
        is.close();

        cout << "Contents \n" << buf << endl;
    }
    // create a block
    Block b(buf, 4096);
    cout << "Data ";
    for (auto t : b.get_data()) {
        //      cout << t;
    }
    cout <<"\n -------------NOW WRITING----------------" << endl;
    BlockCache bc("/foo/bar");
    bc.write("/foo/thing", buf, 4096, 0);

    uint8_t* bufr = new uint8_t[4096];
    bc.read("/foo/thing", bufr, 4086, 10);
    cout << "\n";
}
