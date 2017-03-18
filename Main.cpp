#include "tire_fire.h"
#include "includes.h"
#include <string>
using namespace std;
int
main () {
    string path = "/home/arvin/test/testbaz";
    TireFire tf(path);
    typedef struct foo {
        int a;
        int b;
    } foo;

    int idx = tf.get_tire(sizeof(foo));

    foo* mem = (foo*)tf.get_memory(idx);
    cout << "changing a... " << endl;
    mem->a = 255;
    mem->b = 0;
    cout << to_string(mem->a) << endl;
    tf.flush_head();

    foo meme;
    meme.a = 255;
    meme.b = 0;

    int fd = ::open((path + ".test").c_str(), O_CREAT | O_RDWR, S_IRWXU);

    pwrite(fd, (char*)&meme, sizeof(foo), 0);

    close(fd);
}
