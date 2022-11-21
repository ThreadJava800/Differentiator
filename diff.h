#ifndef DIFF_H
#define DIFF_H

#include <stdio.h>
#include <math.h>
#include <malloc.h>
#include <ctype.h>
#include <string.h>

const int MAX_WORD_LENGTH = 256;

const double EPSILON = 1e-12;

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
    POW         =  4,
    SIN         =  5,
    COS         =  6,
    LN          =  7,
    OPT_DEFAULT = -1,
};

struct DiffNode_t {
    NodeType_t  type = NODET_DEFAULT;

    union value 
    {
        double      num;
        OpType_t    opt;
        const char *var;
    } value;

    DiffNode_t *left  = nullptr;
    DiffNode_t *right = nullptr;
    DiffNode_t *prev  = nullptr;
};

#define LEFT(node)  node->left
#define RIGHT(node) node->right

#define IS_OP(node)  node->type == OP
#define IS_NUM(node) node->type == NUM
#define IS_VAR(node) node->type == VAR

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

DiffNode_t* diffNodeCtor(DiffNode_t* left, DiffNode_t* right, DiffNode_t* prev, int* err = nullptr);

bool compDouble(const double value1, const double value2);

// READ TREE FROM FILE

int addNodeVal(DiffNode_t* node, char* value);

void addPrevs(DiffNode_t* start);

void parseNode(DiffNode_t** node, FILE* readFile);

int parseEquation(FILE* readFile);

DiffNode_t* nodeCopy(DiffNode_t* nodeToCopy);

// MAKING TREE EASIER

void hangNode(DiffNode_t* node, const DiffNode_t* info);

void easierValVal(DiffNode_t* node);

void easierVarVal(DiffNode_t* node, DiffNode_t* varNode, DiffNode_t* valNode);

void makeNodeEasy(DiffNode_t* node);

void easierEqu(DiffNode_t* start);

// ALL FOR DIFF

void diffMul(DiffNode_t* node);

void diffDiv(DiffNode_t* node);

void diffVarPowVal(DiffNode_t* node);

void diffVarPowVar(DiffNode_t* node);

void diffValPowVar(DiffNode_t* node);

void diffPow(DiffNode_t* node);

void diffCos(DiffNode_t* node);

void diffLn(DiffNode_t* node);

void nodeDiff(DiffNode_t* node);

int equDiff(DiffNode_t* start);

// TEX OUTPUT

int openDiffFile(const char *fileName);

void diffNodeDtor(DiffNode_t* node);

void anyTex(DiffNode_t* node, const char* oper, FILE* file);

void powTex(DiffNode_t* node, FILE* file);

void divTex(DiffNode_t* node, FILE* file);

void nodeToTex(DiffNode_t* node, FILE *file);

int diffToTex(DiffNode_t* startNode, const char* fileName);

//

void printNodeVal(DiffNode_t* node, FILE* file);

void graphNode(DiffNode_t *node, FILE *tempFile);

void graphDump(DiffNode_t *node);

#endif