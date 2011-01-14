#include "precomp.h"
#include "mydata.h"
#include "CodeUnixAnsi/AATree.h"
#include <stdlib.h>

void test()
{
    Segment *nil = NULL;
    AATree<Segment*> segs(nil);

    Segment *s[10];
    for (int i = 0; i < 10; i++)
    {
        s[i] = new Segment(10, i);
    }
    segs.insert(0, s[0]);
    segs.insert(10, s[1]);
    segs.insert(10, s[2]);
    segs.insert(30, s[3]);
}

class AAtest
{
public:
    AAtest() { test(); }
};

AAtest aa;