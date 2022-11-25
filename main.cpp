#include <stdio.h>

#include "diff.h"

int main() {
    // BUG in node replace: same letters on different positions!!

    DiffNode_t* res = openDiffFile("test.txt");
    // tailor(res, 6, 0);
    // drawGraph(res);
    // equTangent(res, 2);
    equDiff(res);
    graphDump(res);

    diffNodeDtor(res);

    return 0;
}