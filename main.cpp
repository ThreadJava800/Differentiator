#include <stdio.h>

#include "diff.h"

int main(int argc, char *argv[]) {
    if (argc == 2) {
        DiffNode_t* res = openDiffFile(argv[1]);
        diffNodeDtor(res);
    } else {
        fprintf(stderr, "Incorrect arguments provided\n");
    }

    return 0;
}