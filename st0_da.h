#ifndef ST0_DA_H_
#define ST0_DA_H_

#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* macro arguments **MUST NOT BE FUNCTIONS** because they can get called
   multiple times and cause nasty bugs */

/*
    typedef struct da_t {
        size_t size;
        size_t cap;
        T      *el;
    } da_t;
*/

/* ######################################################################### */
/* utility macros */
/* ######################################################################### */

#define DA_SIZEOF(a) (sizeof(*(a)->el))
#define DA_SIZE(a)   ((a)->size)
#define DA_CAP(a)    ((a)->cap)
#define DA_EL(a)     ((a)->el)

#define MAX(a, b) ((a) > (b) ? (a) : (b))

/* only changes capacity if required */
#define DA_GROW(a, new_size) do {                               \
    if ((new_size) > DA_CAP(a)) {                               \
        if (DA_CAP(a) == 0) DA_CAP(a) = 1;                      \
        while (DA_CAP(a) < (new_size)) DA_CAP(a) *= 2;          \
        DA_EL(a) = realloc(DA_EL(a), DA_CAP(a) * DA_SIZEOF(a)); \
    }                                                           \
} while (0)

/* only changes capacity if required */
#define DA_SHRINK(a, new_size) do {                             \
    if ((new_size) <= DA_CAP(a) / 4) {                          \
        DA_CAP(a) = MAX((new_size) * 2, 1);                     \
        DA_EL(a) = realloc(DA_EL(a), DA_CAP(a) * DA_SIZEOF(a)); \
    }                                                           \
} while (0)

/* ######################################################################### */
/* dynamic array macros */
/* ######################################################################### */

#ifndef ST0_DA_NO_SHORT_NAMES
    #define da_create    st0_da_create
    #define da_destroy   st0_da_destroy
    #define da_clear     st0_da_clear
    #define da_reserve   st0_da_reserve
    #define da_push_back st0_da_push_back
    #define da_pop_back  st0_da_pop_back
    #define da_insert    st0_da_insert
    #define da_delete    st0_da_delete
#endif /* ST0_DA_NO_SHORT_NAMES */

#define st0_da_create(a, init_cap) do {             \
    DA_SIZE(a) = 0;                                 \
    DA_CAP(a) = MAX((init_cap), 1);                 \
    DA_EL(a) = malloc(DA_CAP(a) * DA_SIZEOF(a));    \
} while (0)

#define st0_da_destroy(a) do {  \
    DA_SIZE(a) = 0;             \
    DA_CAP(a) = 0;              \
    free(DA_EL(a));             \
    DA_EL(a) = NULL;            \
} while (0)

#define st0_da_clear(a) do {                                \
    DA_CAP(a) = 1;                                          \
    DA_EL(a) = realloc(DA_EL(a), DA_CAP(a) * DA_SIZEOF(a)); \
    DA_SIZE(a) = 0;                                         \
} while (0)

#define st0_da_reserve(a, cap) do {                             \
    if ((cap) > DA_CAP(a)) {                                    \
        DA_CAP(a) = (cap);                                      \
        DA_EL(a) = realloc(DA_EL(a), DA_CAP(a) * DA_SIZEOF(a)); \
    }                                                           \
} while (0)

#define st0_da_push_back(a, x) do { \
    DA_GROW(a, DA_SIZE(a) + 1);     \
    DA_EL(a)[DA_SIZE(a)] = (x);     \
    DA_SIZE(a) += 1;                \
} while (0)

#define st0_da_pop_back(a) do {                                 \
    assert(DA_SIZE(a) > 0 && "can't pop from an empty array");  \
    DA_SHRINK(a, DA_SIZE(a) - 1);                               \
    DA_SIZE(a) -= 1;                                            \
} while (0)

#define st0_da_insert(a, pos, x) do {                   \
    size_t p = (pos);                                   \
    assert(p <= DA_SIZE(a) && "invalid insert pos");    \
    DA_GROW(a, DA_SIZE(a) + 1);                         \
    memmove(&DA_EL(a)[(pos) + 1],                       \
            &DA_EL(a)[pos],                             \
            (DA_SIZE(a) - (pos)) * DA_SIZEOF(a));       \
    DA_EL(a)[pos] = (x);                                \
    DA_SIZE(a) += 1;                                    \
} while (0)

#define st0_da_delete(a, pos) do {                      \
    size_t p = (pos);                                   \
    assert(p < DA_SIZE(a) && "invalid delete pos");     \
    memmove(&DA_EL(a)[pos],                             \
            &DA_EL(a)[(pos) + 1],                       \
            (DA_SIZE(a) - (pos) - 1) * DA_SIZEOF(a));   \
    DA_SHRINK(a, DA_SIZE(a) - 1);                       \
    DA_SIZE(a) -= 1;                                    \
} while (0)

#endif /* ST0_DA_H_ */
