#include <stdio.h>

#include "diff.h"

int main() {
    DiffNode_t* res = openDiffFile("test.txt");
    // tailor(res, 6, 0);
    drawGraph(res);
    equTangent(res, 2);
    equDiff(res);
    // graphDump(res);

    diffNodeDtor(res);

    return 0;
}