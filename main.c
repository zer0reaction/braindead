#include <stdio.h>
#include <stdbool.h>
#include <assert.h>

#include "st0_da.h"

typedef struct file_t {
    size_t size;
    size_t cap;
    char   *el;
} file_t;

typedef enum tt_t {
    TT_EMPTY = 0,
    TT_EOF = 1,
    TT_INT_VAL = 2,

    TT_INST_INC = '>',
    TT_INST_DEC = '<',
    TT_DATA_INC = '+',
    TT_DATA_DEC = '-',
    TT_READ_BYTE = ',',
    TT_WRITE_BYTE = '.',
    TT_LOOP_START = '[',
    TT_LOOP_END = ']',
    TT_INT_START = '$'
} tt_t;

typedef struct token_t {
    tt_t type;
    int value;
} token_t;

typedef struct tokens_t {
    size_t    size;
    size_t    cap;
    token_t   *el;
} tokens_t;

bool read_file(file_t *file, char *path) {
    FILE *fp = NULL;
    size_t filesize = 0;

    fp = fopen(path, "r");
    if (!fp) {
        perror("read_file");
        return false;
    }

    fseek(fp, 0, SEEK_END);
    filesize = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    da_clear(file);
    da_reserve(file, filesize + 1);
    file->size = filesize + 1;

    fread(file->el, 1, filesize, fp);
    file->el[filesize] = '\0';

    fclose(fp);
    return true;
}

bool check_loops(file_t *file) {
    size_t i = 0;
    int n = 0;

    for (i = 0; i < file->size; i++) {
        switch (file->el[i]) {
        case '[':
            n++;
            break;
        case ']':
            n--;
            break;
        }

        if (n < 0) return false;
    }

    if (n != 0) return false;

    return true;
}

bool tokenize(tokens_t *tokens, file_t *file) {
    size_t i = 0;
    token_t t = {0};
    token_t clear = {0};

    da_clear(tokens);

    for (i = 0; i < file->size; i++) {
        switch (file->el[i]) {
        case '<':
            t.type = TT_INST_DEC;
            da_push_back(tokens, t);
            t = clear;
            break;
        case '>':
            t.type = TT_INST_INC;
            da_push_back(tokens, t);
            t = clear;
            break;

        case '+':
            t.type = TT_DATA_INC;
            da_push_back(tokens, t);
            t = clear;
            break;
        case '-':
            t.type = TT_DATA_DEC;
            da_push_back(tokens, t);
            t = clear;
            break;

        case ',':
            t.type = TT_READ_BYTE;
            da_push_back(tokens, t);
            t = clear;
            break;
        case '.':
            t.type = TT_WRITE_BYTE;
            da_push_back(tokens, t);
            t = clear;
            break;

        case '[':
            t.type = TT_LOOP_START;
            da_push_back(tokens, t);
            t = clear;
            break;
        case ']':
            t.type = TT_LOOP_END;
            da_push_back(tokens, t);
            t = clear;
            break;

        case '$':
            if (file->el[i + 1] < '0' || file->el[i + 1] > '9') {
                printf("[%lu]: expected digit, found '%c'\n", i + 1, file->el[i + 1]);
                return false;
            }

            t.type = TT_INT_START;
            da_push_back(tokens, t);
            t = clear;
            break;
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            if (tokens->size == 0 ||
                tokens->el[tokens->size - 1].type != TT_INT_START)
            {
                break;
            }

            t.type = TT_INT_VAL;
            t.value *= 10;
            t.value += file->el[i] - '0';

            if (file->el[i + 1] < '0' || file->el[i + 1] > '9') {
                da_push_back(tokens, t);
                t = clear;
            }

            break;
        }
    }

    if (t.type != TT_EMPTY) {
        assert(0 && "unreachable");
    }

    t.type = TT_EOF;
    da_push_back(tokens, t);

    return true;
}

void print_tokens(tokens_t *tokens) {
    for (size_t i = 0; i < tokens->size; i++) {
        if (tokens->el[i].type == TT_INT_VAL) {
            printf("%d", tokens->el[i].value);
        } else if (tokens->el[i].type == TT_EOF) {
            printf("EOF ");
        } else {
            printf("%c", tokens->el[i].type);
        }
    }
    printf("\n");
}

bool compile(tokens_t *tokens, char *path) {
    size_t i = 0;
    FILE *fp = NULL;
    size_t stack[1024] = {0};
    unsigned si = 0;
    unsigned loopn = 0;

    fp = fopen(path, "a");
    if (!fp) {
        perror("compile");
        return false;
    }

    fprintf(fp, ".section .bss\n");
    fprintf(fp, ".lcomm data, 30000\n");
    fprintf(fp, "\n");

    fprintf(fp, ".section .text\n");
    fprintf(fp, ".globl _start\n");
    fprintf(fp, "\n");

    fprintf(fp, "_start:\n");
    fprintf(fp, "    leaq data(%%rip), %%r8\n");

    si = 0;
    loopn = 1;

    for (i = 0; i < tokens->size;) {
        switch (tokens->el[i].type) {
        case TT_EMPTY:
            assert(0 && "empty token");
        case TT_EOF:
            i++;
            break;

        case TT_INST_INC:
            fprintf(fp, "    incq %%r8\n");
            i++;
            break;
        case TT_INST_DEC:
            fprintf(fp, "    decq %%r8\n");
            i++;
            break;

        case TT_DATA_INC:
            fprintf(fp, "    incb (%%r8)\n");
            i++;
            break;
        case TT_DATA_DEC:
            fprintf(fp, "    decb (%%r8)\n");
            i++;
            break;

        case TT_READ_BYTE:
            fprintf(fp, "    movq $0, %%rax\n");
            fprintf(fp, "    movq $0, %%rdi\n");
            fprintf(fp, "    movq %%r8, %%rsi\n");
            fprintf(fp, "    movq $1, %%rdx\n");
            fprintf(fp, "    syscall\n");
            i++;
            break;
        case TT_WRITE_BYTE:
            fprintf(fp, "    movq $1, %%rax\n");
            fprintf(fp, "    movq $1, %%rdi\n");
            fprintf(fp, "    movq %%r8, %%rsi\n");
            fprintf(fp, "    movq $1, %%rdx\n");
            fprintf(fp, "    syscall\n");
            i++;
            break;

        case TT_LOOP_START:
            si++;
            stack[si] = loopn;
            loopn++;

            fprintf(fp, "    cmpb $0, (%%r8)\n");
            fprintf(fp, "    jz .end%lu\n", stack[si]);
            fprintf(fp, ".start%lu:\n", stack[si]);

            i++;
            break;
        case TT_LOOP_END:
            fprintf(fp, "    cmpb $0, (%%r8)\n");
            fprintf(fp, "    jnz .start%lu\n", stack[si]);
            fprintf(fp, ".end%lu:\n", stack[si]);

            si--;
            i++;
            break;

        case TT_INT_START:
            if (tokens->el[i + 1].type != TT_INT_VAL) {
                assert(0 && "'$' is not followed by an integer");
            }

            fprintf(fp, "    movb $%d, (%%r8)\n", tokens->el[i + 1].value);
            i += 2;
            break;
        case TT_INT_VAL:
            assert(0 && "stray int token");

        default:
            assert(0 && "unexpected token type");
        }
    }

    fprintf(fp, "    mov $60, %%rax\n");
    fprintf(fp, "    xor %%rdi, %%rdi\n");
    fprintf(fp, "    syscall\n");

    fclose(fp);
    return true;
}

int main(int argc, char **argv) {
    file_t file = {0};
    tokens_t tokens = {0};

    if (argc != 3) {
        printf("incorrect number of arguments: %d, expected: 3\n", argc);
        return 1;
    }

    if (!read_file(&file, argv[1])) {
        printf("reading input file failed\n");
        return 1;
    }

    if (!check_loops(&file)) {
        printf("incorrect loop usage\n");
        return 1;
    }

    if (!tokenize(&tokens, &file)) {
        printf("tokenization failed\n");
        return 1;
    }

    compile(&tokens, argv[2]);

    da_destroy(&file);
    da_destroy(&tokens);
    return 0;
}
