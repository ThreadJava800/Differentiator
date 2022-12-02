#include "diff.h"

FILE* texFile = nullptr;
int   onClose = atexit(closeLogfile);

DiffNode_t* newNodeOper(OpType_t oper, DiffNode_t* left, DiffNode_t* right) {
    if (!right) return nullptr;

    DiffNode_t* node = diffNodeCtor(left, right, nullptr);
    node->type = OP;
    node->value.opt = oper;

    return node;
}

// SUPPORT

DiffNode_t* diffNodeCtor(DiffNode_t* left, DiffNode_t* right, DiffNode_t* prev, int* err) {
    DiffNode_t* diffNode = (DiffNode_t*) calloc(1, sizeof(DiffNode_t));
    if (!diffNode) {
        if (err) *err |= DIFF_NO_MEM;
        return nullptr;
    }

    diffNode->left  = left;
    diffNode->right = right;
    diffNode->prev  = prev;

    return diffNode;
}

DiffNode_t* newNumNode(DiffNode_t* left, DiffNode_t* right, DiffNode_t* prev, double value) {
    DiffNode_t* node = diffNodeCtor(left, right, prev);
    node->type      = NUM;
    node->value.num = value;

    return node;
}

bool compDouble(const double value1, const double value2) {
    return fabs(value1 - value2) < EPSILON;
}

size_t max(size_t first, size_t second) {
    if (first >= second) return first;
    return second;
}

size_t getTreeDepth(DiffNode_t* node) {
    if (!node) return 0;
    
    size_t leftNode  = getTreeDepth(node->left);
    size_t rightNode = getTreeDepth(node->right);

    return max(leftNode, rightNode) + 1;
}

size_t getMaxTreeWidth(DiffNode_t* node) {
    size_t height = getTreeDepth(node);
    size_t maxW = 1;

    for (size_t i = 1; i <= height; i++) {
        size_t w = getTreeWidth(node);
        maxW = max(maxW, w);
    }

    return maxW;
}

size_t getTreeWidth(DiffNode_t* node) {
    if (!node) return 1;

    return getTreeWidth(L(node)) + getTreeWidth(R(node));
}

bool isNodeInList(const DiffNode_t* node, DiffNode_t** replaced, const int* replacedIndex) {
    if (!node || !replaced || !replacedIndex) return false;

    for (int i = 0; i < *replacedIndex; i++) {
        if (node == replaced[i]) return true;
    }

    return false;
}

bool compareSubtrees(DiffNode_t* node1, DiffNode_t* node2) {
    if (!node1 || !node2) return false;

    if (node1->type != node2->type) return false;

    switch (node1->type) {
            case OP:
                if (node1->value.opt == node2->value.opt) {
                    if (node1->right && node2->right) return compareSubtrees(node1->right, node2->right);
                    if (node2->left  && node2->left)  return compareSubtrees(node1->left,  node2->left);
                }
                break;
            case NUM:
                if (compDouble(node1->value.num, node2->value.num)) return true;
                break;
            case VAR:
                if (node1->value.var == node2->value.var) return true;
                break;
            case NODET_DEFAULT:
            default:
                break;
    }
    return false;
}

size_t factorial(int POW_OP) {
    size_t res = 1;
    for (int i = 2; i <= POW_OP; i++) {
        res *= (size_t) i;
    }

    return res;
}

// PARSER

// G    = E '\0'
// E    = T{['+' |  '-']T}*
// T    = ST{['*' | '/']ST}*
// ST   = P{[^]P}*
// P    = '(' E ')' | X
// X    = ['a'-'z' | sin P | cos P | ln P] | N
// N    = ['0'-'9']+

DiffNode_t* setOper(DiffNode_t* val1, DiffNode_t* val2, OpType_t oper) {
    DiffNode_t* operVal = diffNodeCtor(nullptr, nullptr, nullptr);
    operVal->type = OP;
    operVal->value.opt = oper;
    L(operVal) = val1;
    R(operVal) = val2;

    return operVal;
}

DiffNode_t* getN(const char** s) {
    if (!s || !(*s)) return nullptr;

    double val = 0;
    int pointCount = 0, fracNumsCount = 1;
    const char* oldS = *s;

    bool hasFract = false;
    bool isNeg = false;
    if (**s == '-') {
        isNeg = true;
        (*s)++;
    }

    while (('0' <= **s && '9' >= **s) || **s == '.') {
        if (pointCount >= 2) {
            fprintf(stderr, "Incorrect number format.\n");
            return nullptr;
        }
        if (**s == '.') {
            hasFract = true;
            pointCount++;
        } else if (hasFract) {
            val += (**s - '0') / (pow(10, fracNumsCount++));
        }else {
            val = val * 10 + (**s - '0');
        }

        (*s)++;
    }

    if (isNeg) val *= -1;

    if(*s == oldS) return nullptr;

    DiffNode_t* numNode = diffNodeCtor(nullptr, nullptr, nullptr);
    numNode->type = NUM;
    numNode->value.num = val;

    return numNode;
}

DiffNode_t* getP(const char** s) {
    if (!s || !(*s)) return nullptr;

    DiffNode_t* val = nullptr;
    if (**s == '(') {
        (*s)++;
        val = getE(s);
        if(**s != ')') return nullptr;
        (*s)++;
    } else {
        return getX(s);
    }

    return val;
}

DiffNode_t* parseTrig(OpType_t oper, const char** s) {
    if (!s || !(*s)) return nullptr;

    DiffNode_t* rightNode = getP(s);
    return setOper(nullptr, rightNode, oper);
}

DiffNode_t* getX(const char** s) {
    if (!s || !(*s)) return nullptr;

    DiffNode_t* node = nullptr;

    if (!strncmp(*s, "cos", 3)) {
        (*s) += 3;
        return parseTrig(COS_OP, s);
    }
    if (!strncmp(*s, "sin", 3)) {
        (*s) += 3;
        return parseTrig(SIN_OP, s);
    }
    if (!strncmp(*s, "ln", 2)) {
        (*s) += 2;
        return parseTrig(LN_OP, s);
    }

    if ('a' <= **s && **s <= 'z') {
        node = diffNodeCtor(nullptr, nullptr, nullptr);
        node->type = VAR;
        node->value.var = **s;
        (*s)++;
    } else {
        return getN(s);
    }

    return node;
}

DiffNode_t* getT(const char** s) {
    if (!s || !(*s)) return nullptr;

    DiffNode_t* val1 = getSt(s);
    if (!(**s == '*' || **s == '/')) return val1;

    while (**s == '*' || **s == '/') {
        char oper = **s;
        (*s)++;

        DiffNode_t* val2 = getSt(s);

        if (oper == '*') {
            val1 = setOper(val1, val2, MUL_OP);
        } else {
            val1 = setOper(val1, val2, DIV_OP);
        }
    }

    return val1;
}

DiffNode_t* getSt(const char** s) {
    if (!s || !(*s)) return nullptr;

    DiffNode_t* val1 = getP(s);
    if (!(**s == '^')) return val1;

    while (**s == '^') {
        (*s)++;
        DiffNode_t* val2 = getP(s);

        val1 = setOper(val1, val2, POW_OP);
    }

    return val1;
}

DiffNode_t* getE(const char** s) {
    if (!s || !(*s)) return nullptr;

    DiffNode_t* val1 = getT(s);
    if (!(**s == '+' || **s == '-')) return val1;

    while (**s == '+' || **s == '-') {
        char oper = **s;
        (*s)++;

        DiffNode_t* val2 = getT(s);

        if (oper == '+') {
            val1 = setOper(val1, val2, ADD_OP);
        } else {
            val1 = setOper(val1, val2, SUB_OP);
        }
    }

    return val1;
}

DiffNode_t* getG(const char** s) {
    if (!s || !(*s)) return nullptr;

    const char* start = *s;
    DiffNode_t* node = getE(s);
    if (**s != '\0') {
        fprintf(stderr, "Syntax error: (pos=%ld) %s\n", *s - start, *s);
        return nullptr;
    }

    return node;
}

void addPrevs(DiffNode_t* start) {
    if (!start) return;

    if (start->left && start->right) {
        start->left->prev = start->right->prev = start;
    }

    if (start->left)  addPrevs(start->left);
    if (start->right) addPrevs(start->right);
}

long int getFileSize(const char *fileAddress) {
    if (!fileAddress) return 0;

    struct stat fileStat = {};
    stat(fileAddress, &fileStat);

    return fileStat.st_size;
}

char *readTextToBuffer(FILE *file, long int fileSize) {
    if (fileSize < 0) return nullptr;

    char *buffer = (char *) calloc((size_t) fileSize + 1, sizeof(char)); //fileSize can't be negative!
    if (!buffer) return nullptr;

    fread(buffer, sizeof(char), (size_t) fileSize + 1, file); //fileSize can't be negative!

    return buffer;
}

DiffNode_t* parseEquation(const char** s) {
    if (!s) return nullptr;

    DiffNode_t* startNode = getG(s);
    addPrevs(startNode);

    return startNode;
}

DiffNode_t* nodeCopy(DiffNode_t* nodeToCopy) {
    if (!nodeToCopy) return nullptr;

    DiffNode_t* node = (DiffNode_t*) calloc(sizeof(DiffNode_t), 1);
    memcpy(node, nodeToCopy, sizeof(DiffNode_t));
    if (nodeToCopy->left)  {
        node->left = nodeCopy(nodeToCopy->left);
    }
    if (nodeToCopy->right) {
        node->right = nodeCopy(nodeToCopy->right);
    }

    return node;
}

#define replaceValVal(node, oper) {                                       \
    (node)->type = NUM;                                                    \
    (node)->value.num = L(node)->value.num oper R(node)->value.num;         \
    free(L(node));                                                           \
    free(R(node));                                                            \
    L(node)  = nullptr;                                                        \
    R(node) = nullptr;                                                          \
}                                                                                \

void hangNode(DiffNode_t* node, const DiffNode_t* info) {
    if (!node || !info) return;

    node->type  = info->type;
    node->value = info->value;
    node->right = info->right;
    node->left  = info->left;
}

// EASIER SECTION

#define easierTrigVal(node, oper) {                                       \
    node->type = NUM;                                                      \
    node->value.num = oper(R(node)->value.num);                             \
    free(L(node));                                                           \
    free(R(node));                                                            \
    L(node)  = nullptr;                                                        \
    R(node) = nullptr;                                                          \
}                                                                                \

void easierValVal(DiffNode_t* node) {
    if (!node) return;

    switch(node->value.opt) {
        case ADD_OP:
            replaceValVal(node, +);
            break;
        case SUB_OP:
            replaceValVal(node, -);
            break;
        case MUL_OP:
            replaceValVal(node, *);
            break;
        case DIV_OP:
            replaceValVal(node, /);
            break;
        case POW_OP:
            node->type = NUM;
            // node->value.num = POW_OP(L(node)->value.num, R(node)->value.num);
            free(L(node));                                                        
            free(R(node));                                                        
            L(node)  = nullptr;                                                     
            R(node) = nullptr;                                                      
            break;
        case LN_OP:
            easierTrigVal(node, log);                                                  
            break;
        case COS_OP:
            easierTrigVal(node, cos);                                                  
            break;
        case SIN_OP:
            easierTrigVal(node, sin);                                                  
            break;
        case OPT_DEFAULT:
        default:
            return;
            break;
    }

    makeNodeEasy(node->prev);
}

void easierVarVal(DiffNode_t* node, DiffNode_t* varNode, DiffNode_t* valNode) {
    if (!node || !varNode) return;
    if (IS_COS_OP(node) || IS_SIN_OP(node) || IS_LN_OP(node)) return;
    if (IS_DIV(node) && IS_NUM(L(node))) return;

    if (compDouble(valNode->value.num, 1) && (IS_MUL_OP(node) || IS_POW_OP(node))) {
        hangNode(valNode->prev, varNode);
        free(valNode);
    } else if (compDouble(valNode->value.num, 0)) {
        node->type = NUM;
        if (IS_POW_OP(node)) {
            node->value.num = 1;
            node->left = node->right = nullptr;
        } else if (IS_MUL_OP(node)) {
            node->value.num = 0;
            node->left = node->right = nullptr;
        } else if (IS_ADD_OP(node)) {
            hangNode(valNode->prev, varNode);
        }
    } else {
        return;
    }

    makeNodeEasy(node->prev);
}

void makeNodeEasy(DiffNode_t* node) {
    if (!node) return;
    if (!node->left || !node->right) return;

    if (IS_NUM(L(node)) && IS_NUM(R(node))) {
        easierValVal(node);
    } else if (IS_NUM(L(node))) {
        easierVarVal(node, R(node), L(node));
    } else if (IS_NUM(R(node))) {
        easierVarVal(node, L(node), R(node));
    } else {
        return;
    }

    makeNodeEasy(node->prev);
}

void easierEqu(DiffNode_t* start) {
    if (!start) return;

    makeNodeEasy(start);
    if (start->left)  easierEqu(start->left);
    if (start->right) easierEqu(start->right);
}

// DIFF SECTION

DiffNode_t* diffPow(DiffNode_t* startNode, FILE* file) {
    if (!startNode) return nullptr;

    DiffNode_t* result = nullptr;

    if ((IS_OP(L(startNode)) || IS_VAR(L(startNode))) && IS_NUM(R(startNode))) {

        double powVal = R(startNode)->value.num--;
        result = MUL(MUL(dL, newNumNode(nullptr, nullptr, nullptr, powVal)), POW(cL, cR));

    } else if ((IS_VAR(L(startNode)) || IS_OP(L(startNode))) && (IS_VAR(R(startNode)) || IS_OP(R(startNode)))) {

        DiffNode_t* diffPart = MUL(LN(L(startNode)), cL);
        result = MUL(nodeCopy(startNode), nodeDiff(diffPart, file));
        diffNodeDtor(diffPart);

    } else if (IS_NUM(L(startNode)) && (IS_OP(R(startNode)) || IS_VAR(R(startNode)))) {

        result = MUL(nodeCopy(startNode), LN(L(startNode)));

    } else if (IS_NUM(L(startNode)) && IS_NUM(R(startNode))) {
        free(L(startNode));
        free(R(startNode));
        startNode->type = NUM;
        startNode->value.num  = 0;
        L(startNode) = R(startNode) = nullptr;
        return startNode;
    }
    return result;
}

DiffNode_t* nodeDiff(DiffNode_t* node, FILE* file) {
    if (!node) return nullptr;

    DiffNode_t* startNode = nodeCopy(node);

    if (startNode->type == NUM) {
        startNode->value.num = 0;
        return startNode;
    } else if (startNode->type == VAR) {
        startNode->type = NUM;
        startNode->value.num  = 1;
        return startNode;
    } else {
        switch(startNode->value.opt) {
            case ADD_OP:
                startNode = ADD(dL, dR);
                break;
            case SUB_OP:
                startNode = SUB(dL, dR);
                break;
            case MUL_OP:
                startNode = ADD(MUL(dL, cR), MUL(cL, dR));
                break;
            case DIV_OP:
                startNode = DIV(SUB(MUL(dL, cR), MUL(cL, dR)), MUL(cR, cR));
                break;
            case POW_OP:
                startNode = diffPow(startNode, file);
                break;
            case SIN_OP:
                startNode = MUL(COS(cR), dR);
                break;
            case COS_OP:
                startNode = MUL(MUL(newNumNode(nullptr, nullptr, nullptr, -1), SIN(cR)), dR);
                break;
            case LN_OP:
                startNode = DIV(dR, cR);
                break;
            case OPT_DEFAULT:
            default:
                break;
        }
    }
    if (file) {
        printRandomPhrase(file);
        printLineToTex(file, "$(");
        nodeToTex(node, file);
        printLineToTex(file, ")'$ = ");
        diffToTex(startNode);
        printLineToTex(file, "\n\n");
    }
    return startNode;
}

int equDiff(DiffNode_t* start) {
    DIFF_CHECK(!start, DIFF_NULL);

    DiffNode_t* res = nodeDiff(start, texFile);
    addPrevs(res);

    fprintf(texFile, "\\bigskip После очевидных упрощений имеем:\n\n");
    easierEqu(res);
    diffToTex(res);

    return DIFF_OK;
}

DiffNode_t* openDiffFile(const char *fileName, const char *texName) {
    if (!fileName) return nullptr;

    FILE* readFile = fopen(fileName, "rb");
    texFile  = fopen(texName, "w");
    initTex(texFile);
    if (!readFile || !texFile) return nullptr;

    srand((unsigned int) time(NULL));

    const char* equTxt = readTextToBuffer(readFile, getFileSize(fileName));
    DiffNode_t* root = parseEquation(&equTxt);
    fprintf(texFile, "Дано: ");
    diffToTex(root);
    fclose(readFile);

    return root;
}

void diffNodeDtor(DiffNode_t* node) {
    if (!node) return;

    if (node->left)  diffNodeDtor(node->left);
    if (node->right) diffNodeDtor(node->right);

    free(node);
}

// TEX

void anyTex(DiffNode_t* node, const char* oper, FILE* file) {
    if (!node || !oper || !file) return;

    bool needOper = !(IS_NUM(L(node)) && IS_VAR(R(node)) && IS_MUL_OP(node));
    bool needLeftBracket  = !(IS_NUM(L(node))  || IS_VAR(L(node)))  && ((L(node))->texSymb == '\0') 
                                                                          && (IS_MUL_OP(node)) && !isMulSubtree(L(node));
    bool needRightBracket = !(IS_NUM(R(node)) || IS_VAR(R(node))) && ((R(node))->texSymb == '\0')
                                                                          && (IS_MUL_OP(node)) && !isMulSubtree(R(node));

    if (needLeftBracket) fprintf(file, "(");
    printNodeReplaced(node->left, file);
    if (needLeftBracket) fprintf(file, ")");

    if (needOper) fprintf(file, "%s", oper);

    if (needRightBracket) fprintf(file, "(");
    printNodeReplaced(node->right, file);
    if (needRightBracket) fprintf(file, ")");
}

void powTex(DiffNode_t* node, FILE* file) {
    if (!node || !file) return;

    bool needLeftBracket  = L(node)->texSymb == '\0'  && IS_OP(L(node))
                        && (!isMulSubtree(L(node)) || IS_TRIG_LN(L(node)) || !IS_POW_OP(L(node))) ;
    bool needRightBracket = R(node)->texSymb == '\0'  && IS_OP(R(node)) 
                        && (!isMulSubtree(R(node)) || IS_TRIG_LN(R(node)) || !IS_POW_OP(R(node)));

    fprintf(file, "{");
    if (needLeftBracket) fprintf(file, "(");
    printNodeReplaced(node->left, file);
    if (needLeftBracket) fprintf(file, ")");

    fprintf(file, "}^{");

    if (needRightBracket) fprintf(file, "(");
    printNodeReplaced(node->right, file);
    if (needRightBracket) fprintf(file, ")");
    fprintf(file, "}");
}

void divTex(DiffNode_t* node, FILE* file) {
    if (!node || !file) return;

    fprintf(file, "\\frac{");
    printNodeReplaced(node->left, file);
    fprintf(file, "}{");
    printNodeReplaced(node->right, file);
    fprintf(file, "}");
}

void triglogTex(DiffNode_t* node, FILE* file, const char* prep) {
    if (!node || !file) return;

    fprintf(file, "%s(", prep);
    printNodeReplaced(node->right, file);
    fprintf(file, ")");
}

void nodeToTex(DiffNode_t* node, FILE *file) {
    if (!node || !file) return;

    switch (node->type) {
        case OP: 
            {
                switch (node->value.opt) {
                    case MUL_OP:
                        anyTex(node, " \\cdot ", file);
                        break;
                    case DIV_OP:
                        divTex(node, file);
                        break;
                    case SUB_OP:
                        anyTex(node, " - ", file);
                        break;
                    case ADD_OP:
                        anyTex(node, " + ", file);
                        break;
                    case POW_OP:
                        powTex(node, file);
                        break;
                    case COS_OP:
                        triglogTex(node, file, "cos");
                        break;
                    case SIN_OP:
                        triglogTex(node, file, "sin");
                        break;
                    case LN_OP:
                        triglogTex(node, file, "ln");
                        break;
                    case OPT_DEFAULT:
                    default:
                        break;
                }
            };
            break;
        case NUM:
            if (node->value.num > 0 || compDouble(node->value.num, 0)) fprintf(file, "%lg", node->value.num);
            else fprintf(file, "(%lg)", node->value.num);
            break;
        case VAR:
            fprintf(file, "%c",  node->value.var);
            break;
        case NODET_DEFAULT:
            break;
        default:
            break;
    }
}

bool isMulSubtree(DiffNode_t* node) {
    if (!node) return false;

    if (!IS_MUL_OP(node) && !IS_POW_OP(node) && !IS_TRIG_LN(node) && !IS_NUM(node) && !IS_VAR(node)) return false;
    if (L(node)) return isMulSubtree(L(node));
    if (R(node)) return isMulSubtree(R(node));

    return true;
}

void printNodeReplaced(DiffNode_t* node, FILE* file) {
    if (!node || !file) return;

    if (node->texSymb != '\0') {
        fprintf(file, "%c", node->texSymb);
        return;
    }
    nodeToTex(node, file);
}

void printTexReplaced(DiffNode_t* node, FILE* file, DiffNode_t** replaced, int replacedSize) {
    if (!node || !file || !replaced) return;

    fprintf(file, "$");
    printNodeReplaced(node, file);
    fprintf(file, "$");

    if (replacedSize != 0) {
        fprintf(file, ", где:\n\n");
    }
    for (int i = 0; i < replacedSize; i++) {
        fprintf(file, "%c = $", 65 + i);
        nodeToTex(replaced[i], file);
        fprintf(file, "$\n\n");
    }
}

void replaceNode(DiffNode_t* node, DiffNode_t** replaced, int* replacedIndex, size_t maxTreeWidth) {
    if (!node || !replaced || !replacedIndex) return;
    if (getTreeDepth(node) < NEED_TEX_REPLACEMENT) return;

    if (getTreeDepth(node) == NEED_TEX_REPLACEMENT && maxTreeWidth >= CRIT_TREE_WIDTH) {
        for (int i = 0; i < *replacedIndex; i++) {
            if (compareSubtrees(node, replaced[i])) {
                node->texSymb = replaced[i]->texSymb;
                return;
            }
        }

        replaced[*replacedIndex] = node;
        node->texSymb = (char) (65 + *replacedIndex);
        (*replacedIndex)++;
        return;
    }

    replaceNode(node->right, replaced, replacedIndex, maxTreeWidth);
    replaceNode(node->left, replaced, replacedIndex, maxTreeWidth);
}

void makeReplacements(DiffNode_t* start, FILE* file) {
    if (!start || !file) return;

    DiffNode_t** replacedNodes = (DiffNode_t**) calloc(MAX_REPLACE_COUNT, sizeof(DiffNode_t*));
    int replacedIndex = 0;
    replaceNode(start, replacedNodes, &replacedIndex, getMaxTreeWidth(start));

    printTexReplaced(start, file, replacedNodes, replacedIndex);
}

void removeLetters(DiffNode_t* start) {
    if (!start) return;

    if (L(start))  removeLetters(L(start));
    if (R(start)) removeLetters(R(start));
    start->texSymb = '\0';
}

int diffToTex(DiffNode_t* startNode) {
    DIFF_CHECK(!startNode, DIFF_NULL);

    makeReplacements(startNode, texFile);
    removeLetters(startNode);

    return DIFF_OK;
}

void initTex(FILE* file) {
    if (!file) return;

    fprintf(texFile, "\\documentclass{article}\n\n");
    fprintf(texFile, "\\usepackage{amssymb, amsmath, multicol}\n");
    fprintf(texFile, "\\usepackage{graphicx}\n");
    fprintf(texFile, "\\usepackage{float}\n");
    fprintf(texFile, "\\usepackage{wrapfig}\n");
    fprintf(texFile, "\\usepackage[utf8]{inputenc}\n");
    fprintf(texFile, "\\usepackage[T1,T2A]{fontenc}\n");
    fprintf(texFile, "\\usepackage[russian]{babel}\n");
    fprintf(texFile, "\\usepackage{minibox}\n");

    fprintf(texFile, "\\title{[АНТИЗОРИЧ]\\\\ Введение в математический анализ. Непрерывность, пределы, дифферинцируемость}\n"
                     "\\author{Владимир Антонович Зорич}\n"
                     "\\date{Декабрь 1985 год}\n");

    fprintf(texFile, "\\begin{document}\n\\maketitle\n\\sloppy\n");
    fprintf(texFile, "\\texttt{Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore "
    "et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex "
    "ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur." 
    "Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum.\\newline"
    "\\hspace*{\\parindent}От своего имени и от имени будущих читателей я благодарю всех, кто "
    "нашел возможность, живя в разных странах, сообщить в издательство "
    "или мне лично о погрешностях (опечатках, ошибках, пропусках), заме"
    "ченных в русском, английском, немецком или китайском изданиях этого "
    "учебника. Замечания учтены и соответствующая правка внесена в текст "
    "предлагаемого шестого русского издания.\\newline"
    "\\hspace*{\\parindent}Как выяснилось, книга пригодилась и физикам — очень этому рад."
    "Во всяком случае я действительно стремился сопровождать формаль"
    "ную теорию содержательными примерами ее применения как внутри "
    "математики, так и вне нее.\\newline"
    "\\hspace*{\\parindent} Шестое издание содержит ряд дополнений, которые, возможно, бу-"
    "дут полезны студентам и преподавателям. Во-первых, это некоторые "
    "материалы реальных лекций (например записи двух вводных обзорных "
    "лекций первого и третьего семестров) и, во-вторых, это математические "
    "сведения (порой актуальные, например связь многомерной геометрии и "
    "теории вероятностей), примыкающие к основному предмету учебника.\\newline"
    "Москва, 2011 год В. Зорич"
    "}\n\\clearpage\n\\center");
}

void printLineToTex(FILE*file, const char* string) {
    if (!file) return;

    fprintf(texFile, "%s", string);
}

void printRandomPhrase(FILE* file) {
    if (!file) return;

    int phLen = sizeof(phrases) / sizeof(phrases[0]);

    fprintf(texFile, "%s", phrases[rand() % phLen]);
}

// OTHERS

void changeVarToNums(DiffNode_t* node, double num) {
    if (!node) return;

    if (IS_VAR(node)) {
        node->type = NUM;
        node->value.num = num;
    }

    changeVarToNums(node->left, num);
    changeVarToNums(node->right, num);
}

#define BASIC_OPER(val1, val2, oper) val1 oper val2

double funcValue(DiffNode_t* node, double x) {
    if (!node) return 0;

    if (IS_NUM(node)) return node->value.num;
    if (IS_VAR(node)) return x;

    int oper = node->value.opt;
    switch(oper) {
        case ADD_OP:
            return BASIC_OPER(funcValue(L(node), x), funcValue(R(node), x), +);
        case MUL_OP:
            return BASIC_OPER(funcValue(L(node), x), funcValue(R(node), x), *);
        case DIV_OP:
            return BASIC_OPER(funcValue(L(node), x), funcValue(R(node), x), /);
        case SUB_OP:
            return BASIC_OPER(funcValue(L(node), x), funcValue(R(node), x), -);
        case POW_OP:
            return pow(funcValue(L(node), x), funcValue(R(node), x));
        case SIN_OP:
            return sin(funcValue(R(node), x));
        case COS_OP:
            return cos(funcValue(R(node), x));
        case LN_OP:
            return log(funcValue(R(node), x));
        default:
            return 0;
    }

}

void tailor(DiffNode_t* node, int pow, double x0) {
    if (!node || pow <= 0) return;

    double funcVal = funcValue(node, x0);
    fprintf(texFile, "\n\n\\bigskip Ну что? Тейлора тебе дать?\n\n\\minibox[frame]{$");
    if (!compDouble(funcVal, 0)) fprintf(texFile, "%lg + ", funcVal);
    DiffNode_t* diffed = node;

    for (int i = 1; i <= pow; i++) {
        diffed = nodeDiff(diffed, nullptr);
        funcVal = funcValue(diffed, x0);
        if (!compDouble(funcVal, 0)) {
            if (!compDouble(x0, 0)) fprintf(texFile, "\\frac{%lg}{%lu} \\cdot {(x-%lg)}^{%d} + ", funcVal, factorial(i), x0, i);
            else fprintf(texFile, "\\frac{%lg}{%lu} \\cdot {x}^{%d} + ", funcVal, factorial(i), i);
        }
    }
    fprintf(texFile, "\\overline{\\overline{o}}({x}^{%d})$}\n\n", pow);
}

void printPlotOper(DiffNode_t* node, const char* oper, FILE* file) {
    if (!node || !oper || !file) return;

    if (!(IS_NUM(L(node)) || IS_VAR(L(node)))) fprintf(file, "(");
    drawNode(node->left, file);
    if (!(IS_NUM(L(node)) || IS_VAR(L(node)))) fprintf(file, ")");

    fprintf(file, "%s", oper);

    if (!(IS_NUM(R(node)) || IS_VAR(R(node)))) fprintf(file, "(");
    drawNode(node->right, file);
    if (!(IS_NUM(R(node)) || IS_VAR(R(node)))) fprintf(file, ")");
} 

void printTrigPlot(DiffNode_t* node, FILE* file, const char* prep) {
    if (!node || !file || !prep) return;

    fprintf(file, "%s(", prep);
    drawNode(node->right, file);
    fprintf(file, ")");
}

void drawNode(DiffNode_t* node, FILE* file) {
    if (!node || !file) return;

    if (node->type == NUM) {
        fprintf(file, "%lg", node->value.num);
    } else if (node->type == VAR) {
        fprintf(file, "%c", node->value.var);
    } else {
        switch (node->value.opt) {
            case MUL_OP:
                printPlotOper(node, " * ", file);
                break;
            case DIV_OP:
                printPlotOper(node, " / ", file);
                break;
            case SUB_OP:
                printPlotOper(node, " - ", file);
                break;
            case ADD_OP:
                printPlotOper(node, " + ", file);
                break;
            case POW_OP:
                printPlotOper(node, " ** ", file);
                break;
            case COS_OP:
                printTrigPlot(node, file, "cos");
                break;
            case SIN_OP:
                printTrigPlot(node, file, "sin");
                break;
            case LN_OP:
                printTrigPlot(node, file, "log");
                break;
            case OPT_DEFAULT:
            default:
                break;
        }
    }
}

void drawGraph(DiffNode_t* node) {
    if (!node) return;

    FILE* file = popen("gnuplot -persistent", "w");
    if (!file) return;

    fprintf(file, "f(x)=");
    drawNode(node, file);
    fprintf(file, "\nset terminal png size 960,720\nset output 'graph.png'\nplot f(x)\nexit\n");
    pclose(file);

    fprintf(texFile, "\\bigskip График функции ");
    diffToTex(node);
    fprintf(texFile, "имеет вид:\n\n");
    fprintf(texFile, "\\begin{figure}[h]"
                        "\\center{\\includegraphics[width=100mm]{graph.png}}"
                        "\\label{fig:t}"
                     "\\end{figure}");
}

void equTangent(DiffNode_t* node, double x0) {
    if (!node) return;

    DiffNode_t* tangent = nodeCopy(node);
    nodeDiff(tangent, nullptr);
    double k = funcValue(tangent, x0);
    double b = funcValue(node, x0) - k * x0;
    fprintf(texFile, "\n\n \\minibox[frame]{\\centerline{Уравнение касательной в точке x=%lg имеет вид:}\\\\\n", x0);
    fprintf(texFile, "\\centerline{y = %lgx + %lg}}\n\n", k, b);

    diffNodeDtor(tangent);
}

// VISUAL DUMP

void printNodeVal(DiffNode_t* node, FILE* file) {
    if (!node || !file) return;

    switch (node->type) {
        case OP: 
            {
                switch (node->value.opt) {
                    case MUL_OP:
                        fprintf(file, "*");
                        break;
                    case DIV_OP:
                        fprintf(file, "/");
                        break;
                    case SUB_OP:
                        fprintf(file, "-");
                        break;
                    case ADD_OP:
                        fprintf(file, "+");
                        break;
                    case POW_OP:
                        fprintf(file, "^");
                        break;
                    case SIN_OP:
                        fprintf(file, "SIN_OP");
                        break;
                    case COS_OP:
                        fprintf(file, "COS_OP");
                        break;
                    case LN_OP:
                        fprintf(file, "LN_OP");
                        break;
                    case OPT_DEFAULT:
                        break;
                    default:
                        break;
                }
            };
            break;
        case NUM:
            fprintf(file, "%lg", node->value.num);
            break;
        case VAR:
            fprintf(file, "%c",  node->value.var);
            break;
        case NODET_DEFAULT:
        default:
            break;
    }
}

void graphNode(DiffNode_t *node, FILE *tempFile) {
    if (!node || !tempFile) return;

    fprintf(
                tempFile, 
                "\tlabel%p[shape=record, style=\"rounded, filled\", fillcolor=red, label=\"{ {val: ",
                node
            );

    printNodeVal(node, tempFile);
    
    fprintf(
                tempFile, 
                " | {left: %p | right: %p} | current: %p | prev %p | type: %d }}\"];\n", 
                node->left,
                node->right,
                node,
                node->prev,
                node->type
            );

    if (node->left) {
        fprintf(tempFile, "\tlabel%p->label%p [color=\"red\", style=\"dashed\",arrowhead=\"none\"]", node->left, node);
    }
    if (node->right) {
        fprintf(tempFile, "\tlabel%p->label%p [color=\"red\", style=\"dashed\",arrowhead=\"none\"]", node->right, node);
    }

    if (node->left) graphNode(node->left, tempFile);

    if (node->right) graphNode(node->right, tempFile);
}

void graphDump(DiffNode_t *node) {
    if (!node) return;

    FILE *tempFile = fopen("temp.dot", "w");
    if (!tempFile) return;
    fprintf(tempFile, "digraph tree {\n");
    fprintf(tempFile, "\trankdir=HR;\n");

    graphNode(node, tempFile);

    fprintf(tempFile, "}");
    fclose(tempFile);

    system("dot -Tsvg temp.dot > graph.png && xdg-open graph.png");
}

void closeLogfile(void) {
    if (texFile) {
        fprintf(texFile, "\n\\end{document}");
        fclose(texFile);

        system("pdflatex zorich.tex > /dev/null 2>&1 && xdg-open zorich.pdf");
    }
}