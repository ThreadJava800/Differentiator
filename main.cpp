#include <stdio.h>

#include "diff.h"

int main(int argc, char *argv[]) {
    if (argc == 2) {
        DiffNode_t* res = openDiffFile(argv[1]);
        if (!res) {
            fprintf(stderr, "File %s not found!\n", argv[1]);
            return 0;
        }

        diffNodeDtor(res);
    } else {
        fprintf(stderr, "Incorrect arguments provided\n");
    }

    return 0;
}