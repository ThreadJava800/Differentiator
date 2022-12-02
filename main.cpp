#include <stdio.h>

#include "diff.h"

int main() {
    DiffNode_t* res = openDiffFile("hardExample.txt");

    tailor(res, 3, 0);
    drawGraph(res);
    equTangent(res, 0);
    equDiff(res);

    diffNodeDtor(res);

    return 0;
}