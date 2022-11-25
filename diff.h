#ifndef DIFF_H
#define DIFF_H

#include <stdio.h>
#include <math.h>
#include <malloc.h>
#include <ctype.h>
#include <string.h>
#include <time.h>

const int MAX_WORD_LENGTH = 4096;

const double EPSILON = 1e-12;

const int MAX_REPLACE_COUNT = 26;

const int NEED_TEX_REPLACEMENT = 6;

const char phrases[][MAX_WORD_LENGTH] = {
    "Совершенно очевидно, что\n\n",
    "Заметим, что\n\n",
    "Вас это не шокирует?\n\n",
    "Очередное халявное преобразование:\n\n",
    "Это преобразование позаимствуем из вступительных испытаний в советские ясли:\n\n",
    "Иииииииииииииии если:\n\n",
    "Ничего не понял, но очень интересно:\n\n",
    "Любому советскому первокласснику очевидно, что\n\n",
};

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

    char texSymb = '\0';
};

#define LEFT(node)  node->left
#define RIGHT(node) node->right

#define IS_OP(node)  node->type == OP
#define IS_NUM(node) node->type == NUM
#define IS_VAR(node) node->type == VAR

#define IS_COS(node) node->value.opt == COS
#define IS_SIN(node) node->value.opt == SIN
#define IS_LN(node) node->value.opt == LN
#define IS_DIV(node) node->value.opt == DIV
#define IS_MUL(node) node->value.opt == MUL
#define IS_ADD(node) node->value.opt == ADD
#define IS_POW(node) node->value.opt == POW

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

size_t max(size_t first, size_t second);

size_t getTreeDepth(DiffNode_t* node);

bool isNodeInList(const DiffNode_t* node, DiffNode_t** replaced, const int* replacedIndex);

bool compareSubtrees(DiffNode_t* node1, DiffNode_t* node2);

size_t factorial(int pow);

// READ TREE FROM FILE

int addNodeVal(DiffNode_t* node, char* value);

void addPrevs(DiffNode_t* start);

void parseNode(DiffNode_t** node, FILE* readFile);

DiffNode_t* parseEquation(FILE* readFile);

DiffNode_t* nodeCopy(DiffNode_t* nodeToCopy);

// MAKING TREE EASIER

void hangNode(DiffNode_t* node, const DiffNode_t* info);

void easierValVal(DiffNode_t* node);

void easierVarVal(DiffNode_t* node, DiffNode_t* varNode, DiffNode_t* valNode);

void makeNodeEasy(DiffNode_t* node);

void easierEqu(DiffNode_t* start);

// ALL FOR DIFF

void diffMul(DiffNode_t* node, FILE* file);

void diffDiv(DiffNode_t* node, FILE* file);

void diffVarPowVal(DiffNode_t* node, FILE* file);

void diffVarPowVar(DiffNode_t* node, FILE* file);

void diffValPowVar(DiffNode_t* node, FILE* file);

void diffPow(DiffNode_t* node, FILE* file);

void diffCos(DiffNode_t* node, FILE* file);

void diffSin(DiffNode_t* node, FILE* file);

void diffLn(DiffNode_t* node, FILE* file);

void nodeDiff(DiffNode_t* node, FILE* file);

int equDiff(DiffNode_t* start);

DiffNode_t* openDiffFile(const char *fileName, const char *texName = "zorich.tex");

void diffNodeDtor(DiffNode_t* node);

// TEX OUTPUT

void anyTex(DiffNode_t* node, const char* oper, FILE* file);

void powTex(DiffNode_t* node, FILE* file);

void divTex(DiffNode_t* node, FILE* file);

void triglogTex(DiffNode_t* node, FILE* file, const char* prep);

void nodeToTex(DiffNode_t* node, FILE *file);

void printNodeReplaced(DiffNode_t* node, FILE* file);

void printTexReplaced(DiffNode_t* node, FILE* file, DiffNode_t** replaced, int replacedSize);

void replaceNode(DiffNode_t* node, DiffNode_t** replaced, int* replacedIndex);

void makeReplacements(DiffNode_t* start, FILE* file);

int diffToTex(DiffNode_t* startNode);

void initTex(FILE* file);

void printLineToTex(FILE* file, const char* string);

void printRandomPhrase(FILE* file);

// OTHER FUNCS

void changeVarToNums(DiffNode_t* node, double num);

double funcValue(DiffNode_t* node, double x);

void tailor(DiffNode_t* node, int pow, double x0);

void printPlotOper (DiffNode_t* node, const char* oper, FILE* file);

void printTrigPlot(DiffNode_t* node, FILE* file, const char* prep);

void drawNode(DiffNode_t* node, FILE* file);

void drawGraph(DiffNode_t* node);

//

void printNodeVal(DiffNode_t* node, FILE* file);

void graphNode(DiffNode_t *node, FILE *tempFile);

void graphDump(DiffNode_t *node);

void closeLogfile(void);

#endif