#include <iostream>
#include "bplus.h"
using namespace std;
int
main() {
    for (int jj = 5; jj < 50; jj++) {
        cout << "JJ"<<to_string(jj) << endl;
        BPLUSTree bpt;

        for (int ii = 0; ii <= jj; ii++)
            bpt.insert(ii,ii);

        bpt.print();


        cout << to_string(bpt.find(1)) << endl;
        cout << to_string(bpt.find(3)) << endl;
        cout << to_string(bpt.find(7)) << endl;
        //cout << to_string(bpt.find(10)) << endl;
        cout << to_string(bpt.find(9)) << endl;
        //cout << to_string(bpt.find(24)) << endl;
        //cout << to_string(bpt.find(34)) << endl;
        cout << to_string(bpt.find(14)) << endl;

    }
}
