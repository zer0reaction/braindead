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
    TT_CHAR = 3,

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
    unsigned char value;
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
    bool comment = false;

    da_clear(tokens);

    comment = false;

    for (i = 0; i < file->size; i++) {
        if (comment && file->el[i] != '*') continue;

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
            t.type = TT_INT_VAL;
            t.value *= 10;
            t.value += file->el[i] - '0';

            if (file->el[i + 1] < '0' || file->el[i + 1] > '9') {
                da_push_back(tokens, t);
                t = clear;
            }

            break;

        case '/':
            if (file->el[i + 1] == '*') comment = true;
            break;

        case '*':
            if (file->el[i + 1] == '/' && !comment) {
                printf("comment termination without comment\n");
                return false;
            } else if (file->el[i + 1] == '/') {
                comment = false;
            }
            break;

        case '\0':
            t.type = TT_EOF;
            da_push_back(tokens, t);
            t = clear;
            break;

        case ' ':
        case '\t':
        case '\n':
            break;

        default:
            t.type = TT_CHAR;
            t.value = file->el[i];
            da_push_back(tokens, t);
            t = clear;
        }
    }

    if (t.type != TT_EMPTY) {
        assert(0 && "unreachable");
    }

    if (comment) {
        printf("unterminated comment\n");
        return false;
    }

    return true;
}

void print_tokens(tokens_t *tokens) {
    for (size_t i = 0; i < tokens->size; i++) {
        if (tokens->el[i].type == TT_INT_VAL) {
            printf("%d", tokens->el[i].value);
        } else if (tokens->el[i].type == TT_EOF) {
            printf("EOF ");
        } else if (tokens->el[i].type == TT_CHAR) {
            printf("%c", tokens->el[i].value);
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

    fprintf(fp, "getchar:\n");
    fprintf(fp, "    movq $0, %%rax\n");
    fprintf(fp, "    movq $0, %%rdi\n");
    fprintf(fp, "    movq %%r8, %%rsi\n");
    fprintf(fp, "    movq $1, %%rdx\n");
    fprintf(fp, "    syscall\n");
    fprintf(fp, "    ret\n");
    fprintf(fp, "\n");

    fprintf(fp, "putchar:\n");
    fprintf(fp, "    movq $1, %%rax\n");
    fprintf(fp, "    movq $1, %%rdi\n");
    fprintf(fp, "    movq %%r8, %%rsi\n");
    fprintf(fp, "    movq $1, %%rdx\n");
    fprintf(fp, "    syscall\n");
    fprintf(fp, "    ret\n");
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
        case TT_INST_DEC: {
            size_t tcount = 0;
            int res = 0;

            while (tokens->el[i + tcount].type == TT_INST_INC ||
                   tokens->el[i + tcount].type == TT_INST_DEC)
            {
                if (tokens->el[i + tcount].type == TT_INST_INC) res++;
                else                                            res--;
                tcount++;
            }

            if (res != 0) {
                fprintf(fp, "    /* instruction */\n");
                fprintf(fp, "    addq $%d, %%r8\n", res);
            }

            i += tcount;
        } break;

        case TT_DATA_INC:
        case TT_DATA_DEC: {
            size_t tcount = 0;
            int res = 0;

            while (tokens->el[i + tcount].type == TT_DATA_INC ||
                   tokens->el[i + tcount].type == TT_DATA_DEC)
            {
                if (tokens->el[i + tcount].type == TT_DATA_INC) res++;
                else                                            res--;
                tcount++;
            }

            if (res != 0) {
                fprintf(fp, "    /* data */\n");
                fprintf(fp, "    addb $%d, (%%r8)\n", res);
            }

            i += tcount;
        } break;

        case TT_READ_BYTE:
            fprintf(fp, "    /* , */\n");
            fprintf(fp, "    call getchar\n");
            i++;
            break;

        case TT_WRITE_BYTE:
            fprintf(fp, "    /* . */\n");
            fprintf(fp, "    call putchar\n");
            i++;
            break;

        case TT_LOOP_START:
            si++;
            stack[si] = loopn;
            loopn++;

            fprintf(fp, "    /* [ */\n");
            fprintf(fp, "    cmpb $0, (%%r8)\n");
            fprintf(fp, "    jz .end%lu\n", stack[si]);
            fprintf(fp, ".start%lu:\n", stack[si]);

            i++;
            break;

        case TT_LOOP_END:
            fprintf(fp, "    /* ] */\n");
            fprintf(fp, "    cmpb $0, (%%r8)\n");
            fprintf(fp, "    jnz .start%lu\n", stack[si]);
            fprintf(fp, ".end%lu:\n", stack[si]);

            si--;

            i++;
            break;

        case TT_INT_START:
            if (tokens->el[i + 1].type != TT_INT_VAL) {
                printf("'$' is not followed by an integer\n");
                fclose(fp);
                return false;
            }

            fprintf(fp, "    /* $ */\n");
            fprintf(fp, "    movb $%d, (%%r8)\n", tokens->el[i + 1].value);

            i += 2;
            break;

        case TT_INT_VAL:
            switch (tokens->el[i + 1].type) {
            case TT_INST_INC:
                fprintf(fp, "    /* n> */\n");
                fprintf(fp, "    addq $%d, %%r8\n", tokens->el[i].value);
                i += 2;
                break;
            case TT_INST_DEC:
                fprintf(fp, "    /* n< */\n");
                fprintf(fp, "    subq $%d, %%r8\n", tokens->el[i].value);
                i += 2;
                break;
            case TT_DATA_INC:
                fprintf(fp, "    /* n+ */\n");
                fprintf(fp, "    addb $%d, (%%r8)\n", tokens->el[i].value);
                i += 2;
                break;
            case TT_DATA_DEC:
                fprintf(fp, "    /* n- */\n");
                fprintf(fp, "    subb $%d, (%%r8)\n", tokens->el[i].value);
                i += 2;
                break;
            default:
                printf("invalid token after int\n");
                fclose(fp);
                return false;
            }
            break;

        case TT_CHAR:
            fprintf(fp, "    /* char */\n");
            fprintf(fp, "    movb $%d, (%%r8)\n", tokens->el[i].value);
            i++;
            break;

        default:
            assert(0 && "unexpected token type");
        }
    }

    fprintf(fp, "    /* exit(0) */\n");
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
        goto defer_err;
    }

    if (!read_file(&file, argv[1])) {
        printf("reading input file failed\n");
        goto defer_err;
    }

    if (!check_loops(&file)) {
        printf("incorrect loop usage\n");
        goto defer_err;
    }

    if (!tokenize(&tokens, &file)) {
        printf("tokenization failed\n");
        goto defer_err;
    }

    if (!compile(&tokens, argv[2])) {
        printf("compilation failed\n");
        goto defer_err;
    }

    if (file.el) da_destroy(&file);
    if (tokens.el) da_destroy(&tokens);
    return 0;

defer_err:
    if (file.el) da_destroy(&file);
    if (tokens.el) da_destroy(&tokens);
    return 1;
}
