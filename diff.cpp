#include "diff.h"

DiffNode_t* diffNodeCtor(DiffNode_t* left, DiffNode_t* right, int* err) {
    DiffNode_t* diffNode = (DiffNode_t*) calloc(1, sizeof(DiffNode_t));
    if (!diffNode) {
        if (err) *err |= DIFF_NO_MEM;
        return nullptr;
    }

    diffNode->left  = left;
    diffNode->right = right;

    return diffNode;
}

int addNodeVal(DiffNode_t* node, char* value) {
    DIFF_CHECK(!node, DIFF_NULL);
    DIFF_CHECK(!value, DIFF_FILE_NULL);

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
        node->var  = strdup(value);
    }

    return DIFF_OK;
}

void parseNode(DiffNode_t** node, FILE* readFile) {
    if (!readFile || !node) return;

    int symb = getc(readFile);
    if (symb == ')') {
        *node = nullptr;
        return;
    } else if (symb == '(') {
        DiffNode_t* newNode = diffNodeCtor(nullptr, nullptr);
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

    DiffNode_t* startNode = diffNodeCtor(nullptr, nullptr);
    parseNode(&startNode, readFile);
    graphDump(startNode);
    //diffToTex(startNode, "new.tex");

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

void nodeToTex(DiffNode_t* node, FILE *file) {
    if (!node || !file) return;

    fprintf(file, "(");
    if (node->left) nodeToTex(node->left, file);

    printNodeVal(node, file);

    if (node->right) nodeToTex(node->right, file);
    fprintf(file, ")");
}

int diffToTex(DiffNode_t* startNode, const char* fileName) {
    DIFF_CHECK(!startNode, DIFF_NULL);
    DIFF_CHECK(!fileName, DIFF_FILE_NULL);

    FILE *tex = fopen(fileName, "w");
    DIFF_CHECK(!tex, DIFF_FILE_NULL);

    fprintf(tex, "\\documentclass{article}\n");
    fprintf(tex, "\\usepackage{amssymb, amsmath, multicol}\n");
    fprintf(tex, "\\begin{document}\n");

    nodeToTex(startNode, tex);

    fprintf(tex, "\n\\end{document}");

    return DIFF_OK;
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
                " | {left: %p | right: %p} | current: %p }}\"];\n", 
                node->left,
                node->right,
                node
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