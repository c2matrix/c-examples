#include <string.h>
#include "common.h"
#include "vector.h"

vector *v_init(size_t size) {
    vector *v = malloc(sizeof(vector));
    v->array = malloc(NPTRS(size));
    v->size = size;
    v->used = 0;
    return v;
}

void v_grow(vector *v, size_t req) {
    size_t s1 = v->size + v->size / 2;
    size_t new_size = s1 > req ? s1 : req;
    v->array = realloc(v->array, NPTRS(new_size));
    v->size = new_size;
}

void v_add_all(vector *v, ptr *base, size_t n) {
    size_t req = n + v->used;
    if (req > v->size) {
        v_grow(v, req);
    }
    memcpy(&v->array[v->used], base, NPTRS(n));
    v->used = req;
}

void v_add(vector *v, ptr p) {
    v->used++;
    if (v->used > v->size) {
        v_grow(v, 0);
    }
    v->array[v->used - 1] = p;
}

ptr v_pop(vector *v) {
    if (v->used == 0) {
        error("Stack underflow!");
    }
    return v->array[--v->used];
}

ptr v_peek(vector *v) {
    if (v->used == 0) {
        error("Stack underflow!");
    }
    return v->array[v->used - 1];
}

void v_free(vector *v) {
    free(v->array);
    free(v);
}
