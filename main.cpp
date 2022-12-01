#include <stdio.h>

#include "diff.h"

int main() {
    DiffNode_t* res = openDiffFile("test.txt");
    // tailor(res, 7, 3);
    drawGraph(res);
    // equTangent(res, 0);
    // graphDump(res);
    equDiff(res);

    diffNodeDtor(res);

    return 0;
}