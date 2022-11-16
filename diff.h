#ifndef DIFF_H
#define DIFF_H

#include <stdio.h>
#include <math.h>
#include <malloc.h>

enum DiffError_t {
    DIFF_OK         = 0,
    DIFF_FILE_NULL  = 1,
    DIFF_NULL       = 2,
    DIFF_VALUE_NULL = 3,
    DIFF_NO_MEM     = 4,
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

#define DIFF_CHECK(expression, errorCode) { \
    if (expression) {                        \
        return errorCode;                     \
    }                                          \
}                                               \

DiffNode_t* diffNodeCtor(NodeType_t type, void* value, DiffNode_t* prev, int* err = nullptr);

int parseEquation(DiffNode_t* start, FILE* readFile);

void diffNodeDtor(DiffNode_t* node);

void printNodeVal(DiffNode_t* node, FILE* file);

void graphNode(DiffNode_t *node, FILE *tempFile);

void graphDump(DiffNode_t *node);

#endif