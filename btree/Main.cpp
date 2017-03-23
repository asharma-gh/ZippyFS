#include <iostream>
#include "bplus.h"
using namespace std;
int
main() {
    BPLUSTree bpt;

    for (int ii = 0; ii < 10; ii++)
        bpt.insert(ii,ii);

    bpt.print();
}
