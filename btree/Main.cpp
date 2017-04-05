#include <iostream>
#include "bplus.h"
#define MAX 5
using namespace std;
int
main() {
//   for (int jj = 5; jj < NAX; jj++) {
    //      cout << "JJ"<<to_string(jj) << endl;
    BPLUSTree bpt;

    for (int ii = 0; ii <= MAX; ii++) {
        bpt.insert(ii,ii);

        bpt.print();
    }
    for (int ii = 0; ii <= MAX; ii++)
        cout << to_string(bpt.find(ii)) << endl;
    /*
    cout << to_string(bpt.find(1)) << endl;
    cout << to_string(bpt.find(3)) << endl;
    cout << to_string(bpt.find(7)) << endl;
    //cout << to_string(bpt.find(10)) << endl;
    cout << to_string(bpt.find(9)) << endl;
    //cout << to_string(bpt.find(24)) << endl;
    //cout << to_string(bpt.find(34)) << endl;
    cout << to_string(bpt.find(14)) << endl;

    */
    // }
    bpt.print();
}
