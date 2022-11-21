#include "diff.h"

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

bool compDouble(const double value1, const double value2) {
    return fabs(value1 - value2) < EPSILON;
}

int addNodeVal(DiffNode_t* node, char* value) {
    DIFF_CHECK(!node, DIFF_NULL);
    DIFF_CHECK(!value, DIFF_FILE_NULL);

    if (isdigit(*value)) {
        double doubleVal = NAN;
        sscanf(value, "%le", &doubleVal);
        node->type = NUM;
        node->value.num = doubleVal;
    } else if (*value == '-') {
        if (*(value + 1) != '\0') {
            double doubleVal = NAN;
            sscanf(value, "%le", &doubleVal);
            node->type = NUM;
            node->value.num = doubleVal;
        } else {
            node->type = OP;
            node->value.opt = SUB;
        }
    } else if (strcasecmp(value, "sin") == 0) {
        node->type = OP;
        node->value.opt = SIN;
    } else if (strcasecmp(value, "cos") == 0) {
        node->type = OP;
        node->value.opt = COS;
    } else if (strcasecmp(value, "ln")  == 0) {
        node->type = OP;
        node->value.opt = LN;
    } else {
        switch(*value) {
            case '+':
                node->type = OP;
                node->value.opt = ADD;
                break;
            case '*':
                node->type = OP;
                node->value.opt = MUL;
                break;
            case '/':
                node->type = OP;
                node->value.opt = DIV;
                break;
            case '^':
                node->type = OP;
                node->value.opt = POW;
                break;
            default:
                node->type = VAR;
                node->value.var  = strdup(value);
                break;
        }
    }

    return DIFF_OK;
}

void addPrevs(DiffNode_t* start) {
    if (!start) return;

    if (start->left && start->right) {
        start->left->prev = start->right->prev = start;
    }

    if (start->left)  addPrevs(start->left);
    if (start->right) addPrevs(start->right);
}

void parseNode(DiffNode_t** node, FILE* readFile) {
    if (!readFile || !node) return;

    int symb = getc(readFile);
    if (symb == ')') {
        *node = nullptr;
        return;
    } else if (symb == '(') {
        DiffNode_t* newNode = diffNodeCtor(nullptr, nullptr, nullptr);
        parseNode(&(newNode->left), readFile);

        symb = getc(readFile);
        if (symb != '(' && symb != ')') {
            ungetc(symb, readFile);
        }

        char value[MAX_WORD_LENGTH] = "";
        fscanf(readFile, "%[^()\t\n]", value);

        addNodeVal(newNode, value);

        parseNode(&(newNode->right), readFile);
        *node = newNode;
    } else {
        ungetc(symb, readFile);
    }
}

int parseEquation(FILE *readFile) {
    DIFF_CHECK(!readFile, DIFF_FILE_NULL);

    DiffNode_t* startNode = diffNodeCtor(nullptr, nullptr, nullptr);
    parseNode(&startNode, readFile);
    addPrevs(startNode);

    equDiff(startNode);
    addPrevs(startNode);

    //easierEqu(startNode);
    graphDump(startNode);
    //diffToTex(startNode, "new.tex");

    diffNodeDtor(startNode);

    return DIFF_OK;
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
    (node)->value.num = LEFT(node)->value.num oper RIGHT(node)->value.num;  \
    LEFT(node)  = nullptr;                                                   \
    RIGHT(node) = nullptr;                                                    \
}                                                                              \

void hangNode(DiffNode_t* node, const DiffNode_t* info) {
    if (!node || !info) return;

    node->type  = info->type;
    node->value = info->value;
    node->right = info->right;
    node->left  = info->left;
}

void easierValVal(DiffNode_t* node) {
    if (!node) return;

    switch(node->value.opt) {
        case ADD:
            replaceValVal(node, +);
            break;
        case SUB:
            replaceValVal(node, -);
            break;
        case MUL:
            replaceValVal(node, *);
            break;
        case DIV:
            replaceValVal(node, /);
            break;
        case POW:
            node->type = NUM;
            node->value.num = pow(LEFT(node)->value.num, RIGHT(node)->value.num);
            free(LEFT(node));                                                        
            free(RIGHT(node));                                                        
            LEFT(node)  = nullptr;                                                     
            RIGHT(node) = nullptr;                                                      
            break;
        case OPT_DEFAULT:
        case LN:
        case COS:
        case SIN:
        default:
            break;
    }
}

void easierVarVal(DiffNode_t* node, DiffNode_t* varNode, DiffNode_t* valNode) {
    if (!node || !varNode) return;
    
    if (compDouble(valNode->value.num, 1)) {
        hangNode(valNode->prev, varNode);
        free(valNode);
    } else if (compDouble(valNode->value.num, 0)) {
        node->type = NUM;
        if (node->value.opt == POW) {
            node->value.num = 1;
        } else {
            node->value.num = 0;
        }
        node->left = node->right = nullptr;
    } else {
        return;
    }
}

void makeNodeEasy(DiffNode_t* node) {
    if (!node || !node->left || !node->right) return;

    if (IS_NUM(LEFT(node)) && IS_NUM(RIGHT(node))) {
        easierValVal(node);
    } else if (IS_NUM(LEFT(node))) {
        easierVarVal(node, RIGHT(node), LEFT(node));
    } else if (IS_NUM(RIGHT(node))) {
        easierVarVal(node, LEFT(node), RIGHT(node));
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

void diffMul(DiffNode_t* node) {
    if (!node) return;

    DiffNode_t* interimNode1 = diffNodeCtor(nullptr, nullptr, nullptr);
    DiffNode_t* interimNode2 = diffNodeCtor(nullptr, nullptr, nullptr);
    if (!interimNode1 || !interimNode2) return;

    interimNode1->type = interimNode2->type = OP;
    interimNode1->value.opt  = interimNode2->value.opt  = MUL;
    interimNode1->prev = interimNode2->prev = node;

    interimNode1->right = nodeCopy(node->right);
    DiffNode_t* leftNotDiffed = nodeCopy(node->left);
    nodeDiff(node->left);
    interimNode1->left = node->left;

    interimNode2->left = leftNotDiffed;
    nodeDiff(node->right);
    interimNode2->right = node->right;

    node->value.opt = ADD;
    node->left  = interimNode1;
    node->right = interimNode2;
}

void diffDiv(DiffNode_t* node) {
    if (!node) return;

    DiffNode_t* interimNode1 = diffNodeCtor(nullptr, nullptr, node->prev);
    DiffNode_t* interimNode2 = diffNodeCtor(nullptr, nullptr, node->prev);
    DiffNode_t* subNode      = diffNodeCtor(nullptr, nullptr, node->prev);
    DiffNode_t* powNode      = diffNodeCtor(nullptr, nullptr, node->prev);
    if (!interimNode1 || !interimNode2 || !subNode || !powNode) return;

    interimNode1->type = interimNode2->type = powNode->type = subNode->type = OP;
    interimNode1->value.opt  = interimNode2->value.opt  = MUL;
    powNode->value.opt                                  = POW;
    subNode->value.opt                                  = SUB;

    DiffNode_t* numerator   = nodeCopy(node->left);
    DiffNode_t* denominator = nodeCopy(node->right);
    powNode->left           = nodeCopy(node->right);
    powNode->right          = diffNodeCtor(nullptr, nullptr, node->prev);
    powNode->right->type    = NUM;
    powNode->right->value.num     = 2;

    nodeDiff(node->left);
    interimNode1->left  = node->left;
    interimNode1->right = denominator;

    nodeDiff(node->right);
    interimNode2->left  = numerator;
    interimNode2->right = node->right;

    subNode->left  = interimNode1;
    subNode->right = interimNode2;

    node->value.opt = DIV;
    node->left  = subNode;
    node->right = powNode;
}

void diffVarPowVal(DiffNode_t* node) {
    if (!node) return;

    DiffNode_t* leftCopy  = nodeCopy(node->left);
    DiffNode_t* rightCopy = nodeCopy(node->right);
    DiffNode_t* rightNode = diffNodeCtor(nullptr, nullptr, node->prev);
    rightNode->type = OP;
    rightNode->value.opt  = MUL;

    nodeDiff(node->left);

    rightNode->type = OP;
    rightNode->value.opt  = POW;
    LEFT(rightNode) = leftCopy;

    RIGHT(rightNode)       = diffNodeCtor(nullptr, nullptr, node->prev);
    RIGHT(rightNode)->type = OP;
    RIGHT(rightNode)->value.opt  = SUB;

    LEFT(RIGHT(rightNode)) = nodeCopy(node->right);
    RIGHT(RIGHT(rightNode)) = diffNodeCtor(nullptr, nullptr, node->prev);
    RIGHT(RIGHT(rightNode))->type = NUM;
    RIGHT(RIGHT(rightNode))->value.num  = 1;

    DiffNode_t* mulLeftNode = diffNodeCtor(nullptr, nullptr, node->prev);
    mulLeftNode->right = rightNode;
    mulLeftNode->left  = rightCopy;

    node->value.opt  = MUL;
    node->right = mulLeftNode;
}

void diffVarPowVar(DiffNode_t* node) {
    if (!node) return;

    // TODO: add ln
}

void diffValPowVar(DiffNode_t* node) {
    if (!node) return;

    node->value.opt = MUL;
    nodeDiff(node->right);
    node->left->value.num = log(node->left->value.num);
}

void diffPow(DiffNode_t* node) {
    if (!node) return;

    if ((IS_OP(LEFT(node)) || IS_VAR(LEFT(node))) && IS_NUM(RIGHT(node))) {
        diffVarPowVal(node);
    } else if ((IS_VAR(LEFT(node)) || IS_OP(LEFT(node))) && (IS_VAR(RIGHT(node)) || IS_OP(RIGHT(node)))) {
        diffVarPowVar(node);
    } else if (IS_NUM(LEFT(node)) && (IS_OP(RIGHT(node)) || IS_VAR(RIGHT(node)))) {
        diffValPowVar(node);
    } else if (IS_NUM(LEFT(node)) && IS_NUM(RIGHT(node))) {
        free(LEFT(node));
        free(RIGHT(node));
        node->type = NUM;
        node->value.num  = 0;
    } else {
        return;
    }
}

void diffCos(DiffNode_t* node) {
    if (!node) return;

    node->value.opt = SIN;
    DiffNode_t* newRight = nodeCopy(node);
    DiffNode_t* newLeft = diffNodeCtor(nullptr, nullptr, nullptr);
    newLeft->type = OP;
    newLeft->value.opt = MUL;
    LEFT(newLeft) = diffNodeCtor(nullptr, nullptr, newLeft);
    LEFT(newLeft)->type = NUM;
    LEFT(newLeft)->value.num = -1;
    
    nodeDiff(node->right);
    RIGHT(newLeft) = node->right;

    node->value.opt = MUL;
    node->right = newRight;
    node->left  = newLeft;
}

void diffLn(DiffNode_t* node) {
    if (!node) return;

    DiffNode_t* rightCopy = nodeCopy(node->right);

    DiffNode_t* divNode  = diffNodeCtor(nullptr, nullptr, nullptr);
    divNode->type = OP;
    divNode->value.opt = DIV;
    LEFT(divNode) = diffNodeCtor(nullptr, nullptr, nullptr);
    LEFT(divNode)->type = NUM;
    LEFT(divNode)->value.num = 1;
    RIGHT(divNode) = nodeCopy(node->right);

    node->value.opt = MUL;
    node->right = divNode;

    nodeDiff(rightCopy);
    node->left = rightCopy;
}

void nodeDiff(DiffNode_t* node) {
    if (!node) return;

    if (node->type == NUM) {
        node->value.num = 0;
        return;
    } else if (node->type == VAR) {
        node->type = NUM;
        node->value.num  = 1;
        return;
    } else {
        switch(node->value.opt) {
            case ADD:
            case SUB:
                nodeDiff(node->left);
                nodeDiff(node->right);
                break;
            case MUL:
                diffMul(node);
                break;
            case DIV:
                diffDiv(node);
                break;
            case POW:
                diffPow(node);
                break;
            case SIN:
                node->value.opt = COS;
                break;
            case COS:
                diffCos(node);
                break;
            case LN:
                diffLn(node);
                break;
            case OPT_DEFAULT:
            default:
                break;
        }
    }
}

int equDiff(DiffNode_t* start) {
    DIFF_CHECK(!start, DIFF_NULL);

    nodeDiff(start);

    return DIFF_OK;
}

int openDiffFile(const char *fileName) {
    DIFF_CHECK(!fileName, DIFF_FILE_NULL);

    FILE* readFile = fopen(fileName, "rb");
    DIFF_CHECK(!readFile, DIFF_FILE_NULL);

    int err = parseEquation(readFile);
    fclose(readFile);

    return err;
}

void diffNodeDtor(DiffNode_t* node) {
    if (!node) return;

    if (node->left)  diffNodeDtor(node->left);
    if (node->right) diffNodeDtor(node->right);

    free(node);
}

void anyTex(DiffNode_t* node, const char* oper, FILE* file) {
    if (!node || !oper || !file) return;

    if (!(IS_NUM(LEFT(node)) || IS_VAR(LEFT(node)))) fprintf(file, "(");
    nodeToTex(node->left, file);
    if (!(IS_NUM(LEFT(node)) || IS_VAR(LEFT(node)))) fprintf(file, ")");

    fprintf(file, "%s", oper);

    if (!(IS_NUM(RIGHT(node)) || IS_VAR(RIGHT(node)))) fprintf(file, "(");
    nodeToTex(node->right, file);
    if (!(IS_NUM(RIGHT(node)) || IS_VAR(RIGHT(node)))) fprintf(file, ")");
}

void powTex(DiffNode_t* node, FILE* file) {
    if (!node || !file) return;

    fprintf(file, "{");
    nodeToTex(node->left, file);
    fprintf(file, "}^{");
    nodeToTex(node->right, file);
    fprintf(file, "}");
}

void divTex(DiffNode_t* node, FILE* file) {
    if (!node || !file) return;

    fprintf(file, "\\frac{");
    nodeToTex(node->left, file);
    fprintf(file, "}{");
    nodeToTex(node->right, file);
    fprintf(file, "}");
}

void nodeToTex(DiffNode_t* node, FILE *file) {
    if (!node || !file) return;

    switch (node->type) {
        case OP: 
            {
                switch (node->value.opt) {
                    case MUL:
                        anyTex(node, " \\cdot ", file);
                        break;
                    case DIV:
                        divTex(node, file);
                        break;
                    case SUB:
                        anyTex(node, " - ", file);
                        break;
                    case ADD:
                        anyTex(node, " + ", file);
                        break;
                    case POW:
                        powTex(node, file);
                        break;
                    case OPT_DEFAULT:
                    default:
                        break;
                }
            };
            break;
        case NUM:
            fprintf(file, "%lf", node->value.num);
            break;
        case VAR:
            fprintf(file, "%s",  node->value.var);
            break;
        case NODET_DEFAULT:
            break;
        default:
            break;
    }
}

int diffToTex(DiffNode_t* startNode, const char* fileName) {
    DIFF_CHECK(!startNode, DIFF_NULL);
    DIFF_CHECK(!fileName, DIFF_FILE_NULL);

    FILE *tex = fopen(fileName, "w");
    DIFF_CHECK(!tex, DIFF_FILE_NULL);

    fprintf(tex, "\\documentclass{article}\n");
    fprintf(tex, "\\usepackage{amssymb, amsmath, multicol}\n");
    fprintf(tex, "\\begin{document}\n$");

    nodeToTex(startNode, tex);

    fprintf(tex, "$\n\\end{document}");

    return DIFF_OK;
}

void printNodeVal(DiffNode_t* node, FILE* file) {
    if (!node || !file) return;

    switch (node->type) {
        case OP: 
            {
                switch (node->value.opt) {
                    case MUL:
                        fprintf(file, "*");
                        break;
                    case DIV:
                        fprintf(file, "/");
                        break;
                    case SUB:
                        fprintf(file, "-");
                        break;
                    case ADD:
                        fprintf(file, "+");
                        break;
                    case POW:
                        fprintf(file, "^");
                        break;
                    case SIN:
                        fprintf(file, "SIN");
                        break;
                    case COS:
                        fprintf(file, "COS");
                        break;
                    case LN:
                        fprintf(file, "LN");
                        break;
                    case OPT_DEFAULT:
                        break;
                    default:
                        break;
                }
            };
            break;
        case NUM:
            fprintf(file, "%lf", node->value.num);
            break;
        case VAR:
            fprintf(file, "%s",  node->value.var);
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
                " | {left: %p | right: %p} | current: %p | prev %p}}\"];\n", 
                node->left,
                node->right,
                node,
                node->prev
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