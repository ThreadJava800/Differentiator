#include "diff.h"

DiffNode_t* diffNodeCtor(DiffNode_t* prev, int* err) {
    DiffNode_t* diffNode = (DiffNode_t*) calloc(1, sizeof(DiffNode_t));
    if (!diffNode) {
        if (err) *err |= DIFF_NO_MEM;
        return nullptr;
    }

    diffNode->prev = prev;

    // switch (type) {
    //     case OP: 
    //         diffNode->opt = *((OpType_t*) value);
    //         break;
    //     case NUM:
    //         diffNode->val = *((double*) value);
    //         break;
    //     case VAR:
    //         diffNode->var = (const char*) value;
    //         break;
    //     case NODET_DEFAULT:
    //         break;
    //     default:
    //         break;
    // }

    return diffNode;
}

int addNodeVal(DiffNode_t* node, const char* value) {
    DIFF_CHECK(!node, DIFF_NULL);
    DIFF_CHECK(!value, DIFF_FILE_NULL);

    printf("%s \n", value);

    return DIFF_OK;
}

int parseNode(DiffNode_t* node, FILE* readFile) {
    DIFF_CHECK(!node, DIFF_NULL);
    DIFF_CHECK(!readFile, DIFF_FILE_NULL);

    int symb = getc(readFile);
    //printf("%c", symb);
    //SKIP_SPACES();

    //if (symb == EOF) return DIFF_OK;

    // TODO: WORK WITH )*( case!!

    if (symb == ')') {
        return DIFF_OK;
    } else if (symb == '(') {
        node->left = diffNodeCtor(node);
        parseNode(node->left, readFile);

        // char word[MAX_WORD_LENGTH] = "";
        // int wordIndex = 0;

        symb = getc(readFile);
        while (symb != '(' && symb != ')') {
            printf("%c", symb);
            symb = getc(readFile);
        }
        ungetc(symb, readFile);
        // word[wordIndex - 1] = '\0';

        // ungetc(symb, readFile);
        // addNodeVal(node, word);

        node->right = diffNodeCtor(node);
        parseNode(node->right, readFile);
    } else {
        //printf("%c", symb);
        ungetc(symb, readFile);
    }

    return DIFF_OK;
}

int parseEquation(FILE *readFile) {
    DIFF_CHECK(!readFile, DIFF_FILE_NULL);

    int error = DIFF_OK;
    DiffNode_t* startNode = diffNodeCtor(nullptr, &error);
    error |= parseNode(startNode, readFile);

    return error;
}

int openDiffFile(const char *fileName) {
    DIFF_CHECK(!fileName, DIFF_FILE_NULL);

    FILE* readFile = fopen(fileName, "rb");
    DIFF_CHECK(!readFile, DIFF_FILE_NULL);

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