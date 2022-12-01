#include "diff.h"

FILE* texFile = nullptr;
int   onClose = atexit(closeLogfile);
const char* S = nullptr;

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

    int val = 0;
    const char* oldS = *s;

    bool isNeg = false;
    if (**s == '-') {
        isNeg = true;
        (*s)++;
    }

    while ('0' <= **s && '9' >= **s) {
        val = val * 10 + (**s - '0');
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

    DiffNode_t* node = getE(s);
    S = *s;
    if (**s != '\0') return nullptr;

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

void diffMul(DiffNode_t* node, FILE* file) {
    if (!node) return;

    DiffNode_t* interimNode1 = diffNodeCtor(nullptr, nullptr, nullptr);
    DiffNode_t* interimNode2 = diffNodeCtor(nullptr, nullptr, nullptr);
    if (!interimNode1 || !interimNode2) return;

    interimNode1->type = interimNode2->type = OP;
    interimNode1->value.opt  = interimNode2->value.opt  = MUL_OP;
    interimNode1->prev = interimNode2->prev = node;

    interimNode1->right = nodeCopy(node->right);
    DiffNode_t* leftNotDiffed = nodeCopy(node->left);
    nodeDiff(node->left, file);
    interimNode1->left = node->left;

    interimNode2->left = leftNotDiffed;
    nodeDiff(node->right, file);
    interimNode2->right = node->right;

    node->value.opt = ADD_OP;
    node->left  = interimNode1;
    node->right = interimNode2;
}

void diffDiv(DiffNode_t* node, FILE* file) {
    if (!node) return;

    DiffNode_t* interimNode1 = diffNodeCtor(nullptr, nullptr, node->prev);
    DiffNode_t* interimNode2 = diffNodeCtor(nullptr, nullptr, node->prev);
    DiffNode_t* subNode      = diffNodeCtor(nullptr, nullptr, node->prev);
    DiffNode_t* powNode      = diffNodeCtor(nullptr, nullptr, node->prev);
    if (!interimNode1 || !interimNode2 || !subNode || !powNode) return;

    interimNode1->type = interimNode2->type = powNode->type = subNode->type = OP;
    interimNode1->value.opt  = interimNode2->value.opt  = MUL_OP;
    powNode->value.opt                                  = POW_OP;
    subNode->value.opt                                  = SUB_OP;

    DiffNode_t* numerator   = nodeCopy(node->left);
    DiffNode_t* denominator = nodeCopy(node->right);
    powNode->left           = nodeCopy(node->right);
    powNode->right          = diffNodeCtor(nullptr, nullptr, node->prev);
    powNode->right->type    = NUM;
    powNode->right->value.num     = 2;

    nodeDiff(node->left, file);
    interimNode1->left  = node->left;
    interimNode1->right = denominator;

    nodeDiff(node->right, file);
    interimNode2->left  = numerator;
    interimNode2->right = node->right;

    subNode->left  = interimNode1;
    subNode->right = interimNode2;

    node->value.opt = DIV_OP;
    node->left  = subNode;
    node->right = powNode;
}

void diffVarPowVal(DiffNode_t* node, FILE* file) {
    if (!node) return;

    DiffNode_t* leftCopy  = nodeCopy(node->left);
    DiffNode_t* rightCopy = nodeCopy(node->right);
    DiffNode_t* rightNode = diffNodeCtor(nullptr, nullptr, node->prev);
    rightNode->type = OP;
    rightNode->value.opt  = MUL_OP;

    nodeDiff(node->left, file);

    rightNode->type = OP;
    rightNode->value.opt  = POW_OP;
    L(rightNode) = leftCopy;

    R(rightNode)       = diffNodeCtor(nullptr, nullptr, node->prev);
    R(rightNode)->type = OP;
    R(rightNode)->value.opt  = SUB_OP;

    L(R(rightNode)) = nodeCopy(node->right);
    R(R(rightNode)) = diffNodeCtor(nullptr, nullptr, node->prev);
    R(R(rightNode))->type = NUM;
    R(R(rightNode))->value.num  = 1;

    DiffNode_t* mulLeftNode = diffNodeCtor(nullptr, nullptr, node->prev);
    mulLeftNode->right = rightNode;
    mulLeftNode->left  = rightCopy;

    node->value.opt  = MUL_OP;
    node->right = mulLeftNode;
}

void diffVarPowVar(DiffNode_t* node, FILE* file) {
    if (!node) return;

    DiffNode_t* rootCopy = nodeCopy(node);
    node->left = nodeCopy(node);
    node->value.opt = MUL_OP;

    DiffNode_t* rightNode = diffNodeCtor(nullptr, nullptr, nullptr);
    DiffNode_t* lnNode    = diffNodeCtor(nullptr, nullptr, nullptr);

    lnNode->type = OP;
    lnNode->value.opt = LN_OP;

    L(lnNode) = diffNodeCtor(nullptr, nullptr, nullptr);
    L(lnNode)->type = NUM;
    L(lnNode)->value.num = 0;
    R(lnNode) = L(rootCopy);

    L(rightNode) = R(rootCopy);
    R(rightNode) = lnNode;

    nodeDiff(rightNode, file);
    node->right = rightNode;
}

void diffValPowVar(DiffNode_t* node, FILE* file) {
    if (!node) return;

    node->value.opt = MUL_OP;
    nodeDiff(node->right, file);
    node->left->value.num = log(node->left->value.num);
}

DiffNode_t* diffPow(DiffNode_t* startNode, FILE* file) {
    if (!startNode) return nullptr;

    DiffNode_t* result = nullptr;

    if ((IS_OP(L(startNode)) || IS_VAR(L(startNode))) && IS_NUM(R(startNode))) {
        double powVal = R(startNode)->value.num--;
        result = MUL(MUL(dL, newNumNode(nullptr, nullptr, nullptr, powVal)), POW(cL, cR));
    } else if ((IS_VAR(L(startNode)) || IS_OP(L(startNode))) && (IS_VAR(R(startNode)) || IS_OP(R(startNode)))) {
        DiffNode_t* diffPart = MUL(LN(L(startNode)), cL);
        result = MUL(nodeCopy(startNode), nodeDiff(diffPart, texFile));
        diffNodeDtor(diffPart);
    } else if (IS_NUM(L(startNode)) && (IS_OP(R(startNode)) || IS_VAR(R(startNode)))) {
        // diffValPowVar(startNode, file);
    } else if (IS_NUM(L(startNode)) && IS_NUM(R(startNode))) {
        // free(L(startNode));
        // free(R(startNode));
        // startNode->type = NUM;
        // startNode->value.num  = 0;
        // L(startNode) = R(startNode) = nullptr;
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
                return ADD(dL, dR);
            case SUB_OP:
                return SUB(dL, dR);
            case MUL_OP:
                return ADD(MUL(dL, cR), MUL(cL, dR));
            case DIV_OP:
                return DIV(SUB(MUL(dL, cR), MUL(cL, dR)), MUL(cR, cR));
            case POW_OP:
                return diffPow(startNode, file);
            case SIN_OP:
                return MUL(COS(cR), dR);
            case COS_OP:
                return MUL(MUL(newNumNode(nullptr, nullptr, nullptr, -1), SIN(cR)), dR);
            case LN_OP:
                return DIV(dR, cR);
            case OPT_DEFAULT:
            default:
                break;
        }
    }
    // if (file) {
    //     printRandomPhrase(file);
    //     printLineToTex(file, "$(");
    //     nodeToTex(startNode, file);
    //     printLineToTex(file, ")'$ = ");
    //     diffToTex(node);
    //     printLineToTex(file, "\n\n");
    //     diffNodeDtor(startNode);
    // }
    return startNode;
}

int equDiff(DiffNode_t* start) {
    DIFF_CHECK(!start, DIFF_NULL);

    DiffNode_t* res = nodeDiff(start, texFile);
    addPrevs(res);

    fprintf(texFile, "\\bigskip После очевидных упрощений имеем:\n\n");
    easierEqu(res);
    diffToTex(res);

    printf("%p ", res);
    graphDump(res);

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
                                                                          && (IS_MUL_OP(node));
    bool needRightBracket = !(IS_NUM(R(node)) || IS_VAR(R(node))) && ((R(node))->texSymb == '\0')
                                                                          && (IS_MUL_OP(node));

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

    bool needLeftBracket  = L(node)->texSymb == '\0'  && IS_OP(L(node));
    bool needRightBracket = R(node)->texSymb == '\0' && IS_OP(R(node));

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
            fprintf(file, "%lg", node->value.num);
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

void replaceNode(DiffNode_t* node, DiffNode_t** replaced, int* replacedIndex) {
    if (!node || !replaced || !replacedIndex) return;
    if (getTreeDepth(node) < NEED_TEX_REPLACEMENT) return;

    if (getTreeDepth(node) == NEED_TEX_REPLACEMENT) {
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

    replaceNode(node->right, replaced, replacedIndex);
    replaceNode(node->left, replaced, replacedIndex);
}

void makeReplacements(DiffNode_t* start, FILE* file) {
    if (!start || !file) return;

    DiffNode_t** replacedNodes = (DiffNode_t**) calloc(MAX_REPLACE_COUNT, sizeof(DiffNode_t*));
    int replacedIndex = 0;
    replaceNode(start, replacedNodes, &replacedIndex);

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
    fprintf(texFile, "\\begin{document}\n");
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

void tailor(DiffNode_t* node, int POW_OP, double x0) {
    if (!node || POW_OP <= 0) return;

    DiffNode_t* tailorCopy = nodeCopy(node);

    fprintf(texFile, "\\bigskip Ну что? Тейлора тебе дать?\n\n$%.2lf + ", funcValue(tailorCopy, x0));
    for (int i = 1; i < POW_OP; i++) {
        nodeDiff(tailorCopy, nullptr);
        fprintf(texFile, "\\frac{%.2lf}{%lu} * {(x-%.2lf)}^{%d} + ", funcValue(tailorCopy, x0), factorial(i), x0, i);
    }
    fprintf(texFile, "\\overline{\\overline{o}}({x}^{%d}) $\n\n", POW_OP);

    diffNodeDtor(tailorCopy);
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
        fprintf(file, "%.2lf", node->value.num);
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
    fprintf(texFile, "Уравнение касательной в точке x=%.2lf имеет вид:\n\n", x0);
    fprintf(texFile, "y = %.2lfx + %.2lf:\n\n", k, b);

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
            fprintf(file, "%.2lf", node->value.num);
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

        system("pdflatex zorich.tex > /dev/null && xdg-open zorich.pdf");
    }
}