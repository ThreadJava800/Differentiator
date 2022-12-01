#include <stdio.h>

#include "diff.h"

extern const char* S;

int main() {
    // Syntax error handler

    // функция предсказывающая длину распечатки с учетом опретора фрак и степени
    DiffNode_t* res = openDiffFile("test.txt");
    if (!res) {
        printf("Syntax error: %s\n", S);
    }
    tailor(res, 7, 3);
    drawGraph(res);
    // equTangent(res, 0);
    // graphDump(res);
    equDiff(res);

    diffNodeDtor(res);

    return 0;
}