#include "diff.h"

DiffNode_t* diffNodeCtor(DiffNode_t* prev, DiffNode_t* left, DiffNode_t* right, int* err) {
    DiffNode_t* diffNode = (DiffNode_t*) calloc(1, sizeof(DiffNode_t));
    if (!diffNode) {
        if (err) *err |= DIFF_NO_MEM;
        return nullptr;
    }

    diffNode->prev  = prev;
    diffNode->left  = left;
    diffNode->right = right;

    return diffNode;
}

int addNodeVal(DiffNode_t* node, char* value) {
    DIFF_CHECK(!node, DIFF_NULL);
    DIFF_CHECK(!value, DIFF_FILE_NULL);

    printf("%s", value);

    if (isdigit(*value)) {
        double doubleVal = NAN;
        sscanf(value, "%le", &doubleVal);
        node->type = NUM;
        node->val = doubleVal;
    } else if (*value == '-') {
        if (*(value + 1) != '\0') {
            double doubleVal = NAN;
            sscanf(value, "%le", &doubleVal);
            node->type = NUM;
            node->val = doubleVal;
        } else {
            node->type = OP;
            node->opt = SUB;
        }
    } else if (*value == '+') {
        node->type = OP;
        node->opt = ADD;
    } else if (*value == '*') {
        node->type = OP;
        node->opt = MUL;
    } else if (*value == '/') {
        node->type = OP;
        node->opt = DIV;
    } else {
        node->type = VAR;
        node->var  = value;
    }

    return DIFF_OK;
}

DiffNode_t* parseNode(FILE* readFile) {
    if (!readFile) return nullptr;

    DiffNode_t* leftNode  = nullptr;
    DiffNode_t* rightNode = nullptr;
    DiffNode_t* newNode   = nullptr;

    int symb = getc(readFile);
    while (symb != EOF) {
        //printf("%c", symb);
        if (symb == '(') {
            newNode = parseNode(readFile);

            if (!leftNode)  leftNode  = newNode;
            else            rightNode = newNode;

            // printf("\n%s %p %p\n", leftNode->var, rightNode, newNode);
        } else {
            char value[MAX_WORD_LENGTH] = "";
            int valueId = 0;
            while (symb != '(' && symb != ')') {
                value[valueId] = (char) symb;
                //printf("%c", symb);
                symb = getc(readFile);
                valueId++;
            }

            newNode = diffNodeCtor(nullptr, nullptr, nullptr);
            // TODO: check for null
            addNodeVal(newNode, value);

            if (newNode->type == OP) {
                newNode->left = leftNode;
                newNode->right = rightNode;
            }
        }
        symb = getc(readFile);
    }

    return newNode;
}

int parseEquation(FILE *readFile) {
    DIFF_CHECK(!readFile, DIFF_FILE_NULL);

    DiffNode_t* startNode = parseNode(readFile);

    return DIFF_OK;
}

int openDiffFile(const char *fileName) {
    DIFF_CHECK(!fileName, DIFF_FILE_NULL);

    FILE* readFile = fopen(fileName, "rb");
    DIFF_CHECK(!readFile, DIFF_FILE_NULL);

    // TODO: close file

    return parseEquation(readFile);
}

void diffNodeDtor(DiffNode_t* node) {
    if (!node) return;

    if (node->left)  diffNodeDtor(node->left);
    if (node->right) diffNodeDtor(node->right);

    free(node);
}

void printNodeVal(DiffNode_t* node, FILE* file) {
    if (!node || !file) return;

    switch (node->type) {
        case OP: 
            {
                switch (node->opt) {
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
                    case OPT_DEFAULT:
                        break;
                    default:
                        break;
                }
            };
            break;
        case NUM:
            fprintf(file, "%lf", node->val);
            break;
        case VAR:
            fprintf(file, "%s",  node->var);
            break;
        case NODET_DEFAULT:
            break;
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
                " | {left: %p | right: %p} | previous: %p }}\"];\n", 
                node->left,
                node->right,
                node->prev
            );

    if (node->prev) {
        fprintf(tempFile, "\tlabel%p->label%p [color=\"red\", style=\"dashed\",arrowhead=\"none\"]", node->prev, node);
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