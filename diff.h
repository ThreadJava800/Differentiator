#ifndef DIFF_H
#define DIFF_H

#include <stdio.h>
#include <math.h>
#include <malloc.h>
#include <ctype.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>

const int MAX_WORD_LENGTH = 4096;

const double EPSILON = 1e-12;

const int MAX_REPLACE_COUNT = 52;

const int NEED_TEX_REPLACEMENT = 11;

const char phrases[][MAX_WORD_LENGTH] = {
    "\\bigskip Совершенно очевидно, что\n\n",
    "\\bigskip Заметим, что\n\n",
    "\\bigskip Вас это не шокирует?\n\n",
    "\\bigskip Очередное халявное преобразование:\n\n",
    "\\bigskip Это преобразование позаимствуем из вступительных испытаний в советские ясли:\n\n",
    "\\bigskip Иииииииииииииии если:\n\n",
    "\\bigskip Ничего не понял, но очень интересно:\n\n",
    "\\bigskip Любому советскому первокласснику очевидно, что\n\n",
    "\\bigskip Ну, а это вообще не должно вызывать вопросов:\n\n",
    "\\bigskip По теореме Дашкова-Гущина:\n\n",
    "\\bigskip Однажды Хемингуэй поспорил, что сможет написать самый трогательный рассказ:\n\n",
    "\\bigskip Ну а это вообще база:\n\n",
    "\\bigskip Я бы давно бы вас убил за это количество переменных А, ещё на стадии двух переменных А!\n\n",
    "\\bigskip KERNEL PANIC - NOT SYNCING!\n\n",
    "\\bigskip Segmentation fault (core dumped)\n\n",
    "\\bigskip Я не знаю почему это так, но это факт:\n\n",
    "\\bigskip Если аксиомы не противоречивы, то\n\n",
    "\\bigskip Если Земля плоская, то очевидно:\n\n",
    "\\bigskip (HONORABLE MENTION)\n\n",
    "\\bigskip Взял с первой страницы в гугле:\n\n",
    "\\bigskip Ну вот! 14 стадий, и матан выучен!\n\n",
    "\\bigskip Даже сишнику очевидно\n\n",
    "\\bigskip Даже физтеху очевидно\n\n",
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
    MUL_OP         =  0,
    ADD_OP         =  1,
    DIV_OP         =  2,
    SUB_OP         =  3,
    POW_OP         =  4,
    SIN_OP         =  5,
    COS_OP         =  6,
    LN_OP          =  7,
    OPT_DEFAULT = -1,
};

struct DiffNode_t {
    NodeType_t  type = NODET_DEFAULT;

    union value 
    {
        double      num;
        OpType_t    opt;
        char        var;
    } value;

    DiffNode_t *left  = nullptr;
    DiffNode_t *right = nullptr;
    DiffNode_t *prev  = nullptr;

    char texSymb = '\0';
};

// FOR DSL

DiffNode_t* newNodeOper(OpType_t oper, DiffNode_t* left, DiffNode_t* right);

#define L(node)    node->left
#define R(node)    node->right
#define OPER(node) node->value.opt
#define RR(node) R(R(node))
#define LL(node) L(L(node))

#define dL nodeDiff(L(startNode), file)
#define cL nodeCopy(L(startNode))
#define dR nodeDiff(R(startNode), file)
#define cR nodeCopy(R(startNode))

#define IS_OP(node)  (node->type == OP)
#define IS_NUM(node) (node->type == NUM)
#define IS_VAR(node) (node->type == VAR)

#define IS_COS_OP(node) node->value.opt == COS_OP
#define IS_SIN_OP(node) node->value.opt == SIN_OP
#define IS_LN_OP(node) node->value.opt == LN_OP
#define IS_DIV(node) node->value.opt == DIV_OP
#define IS_MUL_OP(node) (node->value.opt == MUL_OP)
#define IS_ADD_OP(node) node->value.opt == ADD_OP
#define IS_POW_OP(node) (node->value.opt == POW_OP)
#define IS_TRIG_LN(node) (node->value.opt == COS_OP || node->value.opt == SIN_OP || node->value.opt == LN_OP)

#define ADD(node1, node2) newNodeOper(ADD_OP, node1,   node2)
#define SUB(node1, node2) newNodeOper(SUB_OP, node1,   node2)
#define MUL(node1, node2) newNodeOper(MUL_OP, node1,   node2)
#define DIV(node1, node2) newNodeOper(DIV_OP, node1,   node2)
#define POW(node1, node2) newNodeOper(POW_OP, node1,   node2)
#define COS(node)         newNodeOper(COS_OP, nullptr, node)
#define SIN(node)         newNodeOper(SIN_OP, nullptr, node)
#define LN(node)          newNodeOper(LN_OP,  nullptr, node)

#define SET_MUL_OP(node) {     \
    node->type      = OP;       \
    node->value.opt = MUL_OP;    \
    node->prev      = node;       \
}

#define R_VAR_L_NUM(node) (IS_NUM(L(node))  &&  IS_VAR(R(node)))
#define L_VAR_R_NUM(node) (IS_NUM(R(node)) &&  IS_VAR(L(node)))

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

DiffNode_t* newNumNode(DiffNode_t* left, DiffNode_t* right, DiffNode_t* prev, double value);

bool compDouble(const double value1, const double value2);

size_t max(size_t first, size_t second);

size_t getTreeDepth(DiffNode_t* node);

bool isNodeInList(const DiffNode_t* node, DiffNode_t** replaced, const int* replacedIndex);

bool compareSubtrees(DiffNode_t* node1, DiffNode_t* node2);

size_t factorial(int pow);

// READ TREE FROM FILE

void addPrevs(DiffNode_t* start);

DiffNode_t* getG(const char** s);

DiffNode_t* getE(const char** s);

DiffNode_t* getT(const char** s);

DiffNode_t* getP(const char** s);

DiffNode_t* parseTrig(OpType_t oper, const char** s);

DiffNode_t* getX(const char** s);

DiffNode_t* setOper(DiffNode_t* val1, DiffNode_t* val2, OpType_t oper);

DiffNode_t* getN(const char** s);

DiffNode_t* getSt(const char** s);

long int getFileSize(const char *fileAddress);

char *readTextToBuffer(FILE *file, long int fileSize);

DiffNode_t* parseEquation(const char** s);

DiffNode_t* nodeCopy(DiffNode_t* nodeToCopy);

// MAKING TREE EASIER

void hangNode(DiffNode_t* node, const DiffNode_t* info);

void easierValVal(DiffNode_t* node);

void easierVarVal(DiffNode_t* node, DiffNode_t* varNode, DiffNode_t* valNode);

void makeNodeEasy(DiffNode_t* node);

void easierEqu(DiffNode_t* start);

// ALL FOR DIFF

DiffNode_t* diffPow(DiffNode_t* startNode, FILE* file);

DiffNode_t* nodeDiff(DiffNode_t* startNode, FILE* file);

int equDiff(DiffNode_t* start);

DiffNode_t* openDiffFile(const char *fileName, const char *texName = "zorich.tex");

void diffNodeDtor(DiffNode_t* node);

// TEX OUTPUT

void anyTex(DiffNode_t* node, const char* oper, FILE* file);

void powTex(DiffNode_t* node, FILE* file);

void divTex(DiffNode_t* node, FILE* file);

void triglogTex(DiffNode_t* node, FILE* file, const char* prep);

void nodeToTex(DiffNode_t* node, FILE *file);

bool isMulSubtree(DiffNode_t* node);

void printNodeReplaced(DiffNode_t* node, FILE* file);

void printTexReplaced(DiffNode_t* node, FILE* file, DiffNode_t** replaced, int replacedSize);

void replaceNode(DiffNode_t* node, DiffNode_t** replaced, int* replacedIndex);

void makeReplacements(DiffNode_t* start, FILE* file);

void removeLetters(DiffNode_t* start);

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

void equTangent(DiffNode_t* node, double x0);

//

void printNodeVal(DiffNode_t* node, FILE* file);

void graphNode(DiffNode_t *node, FILE *tempFile);

void graphDump(DiffNode_t *node);

void closeLogfile(void);

#endif