#include "tire_fire.h"
#include "includes.h"
#include <sys/mman.h>
#include <string>
using namespace std;
int
main () {

    typedef struct node {
        // next is one of: idx to next node, or -1
        int next;
        int val;
    } node;

    string path2 =  "/home/arvin/test/st/selfref";
    TireFire tf2(path2);

    int64_t rootidx = tf2.get_tire(sizeof(node));

    node* root = (node*)tf2.get_memory(rootidx);
    cout << "setting root val to 42..." << endl;
    root->val = 42;

    cout << "creating next node..." << endl;

    int64_t childidx = tf2.get_tire(sizeof(node));
    cout << "setting child val to 255..." << endl;

    node* child = (node*)tf2.get_memory(childidx);
    child->val = 255;
    child->next = -1;
    cout << "setting root to point to child..." << endl;
    root->next = childidx;

    cout << "flushing..." << endl;
    tf2.flush_head();

    cout << "reconstructing..." << endl;
    // mmap data file to memory
    int fd = ::open(path2.c_str(), O_RDWR);
    void* ffile = mmap(0, sizeof(node) * 2, PROT_READ, MAP_SHARED, fd, 0);
    // mmap header file to memory
    int hfd = ::open((path2 + ".head").c_str(), O_RDWR);
    uint64_t* hfile = (uint64_t*)mmap(0, sizeof(uint64_t) * 2, PROT_READ, MAP_SHARED, hfd, 0);

    // get root
    cout << "getting root..." << endl;
    node* sroot = (node*)ffile;

    cout << "reading root val..." << endl;
    cout << to_string(sroot->val) << endl;
    cout << "reading root next..." << endl;
    cout << to_string(sroot->next) << endl;
    cout << "getting child..." << endl;
    cout << "idx: " << to_string(hfile[sroot->next])<< endl;
    node* schild = (node*)(hfile[sroot->next] + (char*)ffile);
    cout << "printing child val..." << endl;
    cout << to_string(schild->val) << endl;
    cout << "printing child next..." << endl;
    cout << to_string(schild->next) << endl;

    cout << "flushing stuff" << endl;
    munmap(ffile, sizeof(node) * 2);
    munmap(hfile, sizeof(int) * 2);
    close(fd);
    close(hfd);
}
