#ifndef DIFF_H
#define DIFF_H

#include <stdio.h>
#include <math.h>
#include <malloc.h>
#include <ctype.h>

const int MAX_WORD_LENGTH = 256;

enum DiffError_t {
    DIFF_OK         = 2 << 0,
    DIFF_FILE_NULL  = 2 << 1,
    DIFF_NULL       = 2 << 2,
    DIFF_VALUE_NULL = 2 << 3,
    DIFF_NO_MEM     = 2 << 4,
};

enum NodeType_t {
    OP            =  0,
    VAR           =  1,
    NUM           =  2,
    NODET_DEFAULT = -1,
};

enum OpType_t {
    MUL         =  0,
    ADD         =  1,
    DIV         =  2,
    SUB         =  3,
    OPT_DEFAULT = -1,
};

struct DiffNode_t {
    NodeType_t  type = NODET_DEFAULT;
    double      val  = NAN;
    OpType_t    opt  = OPT_DEFAULT;
    const char *var  = nullptr;

    DiffNode_t *prev  = nullptr;
    DiffNode_t *left  = nullptr;
    DiffNode_t *right = nullptr;
};

#define SKIP_SPACES() {      \
    while (symb == ' ') {     \
        symb = getc(readFile); \
    }                           \
}                                \

#define DIFF_CHECK(expression, errorCode) { \
    if (expression) {                        \
        return errorCode;                     \
    }                                          \
}                                               \

DiffNode_t* diffNodeCtor(DiffNode_t* prev, DiffNode_t* left, DiffNode_t* right, int* err = nullptr);

int addNodeVal(DiffNode_t* node, char* value);

DiffNode_t* parseNode(FILE* readFile);

int parseEquation(FILE* readFile);

int openDiffFile(const char *fileName);

void diffNodeDtor(DiffNode_t* node);

void printNodeVal(DiffNode_t* node, FILE* file);

void graphNode(DiffNode_t *node, FILE *tempFile);

void graphDump(DiffNode_t *node);

#endif