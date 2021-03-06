#include <assert.h>
#include "quickfit/quickfit.h"

size_t
qf_largest_free_block(quick_fit *qf) {
    rbtree *node = rbt_iterate(qf->large_blocks, NULL, BST_RIGHT);
    if (node) {
        return node->key;
    }
    for (int i = QF_N_BUCKETS - 1; i >= 0; i--) {
        vector *small_blocks = qf->buckets[i];
        if (small_blocks->used) {
            return i * QF_DATA_ALIGNMENT;
        }
    }
    return 0;
}

void
test_basic() {
    size_t size = 10 * 1024;
    ptr region = (ptr)malloc(size);
    quick_fit *qf = qf_init(region, size);


    for (int i = 0; i < QF_N_BUCKETS; i++) {
        assert(qf->buckets[i]->used == 0);
    }
    assert(qf->n_blocks == 1);
    assert(rbt_size(qf->large_blocks) == 1);

    ptr p = qf_allot_block(qf, 1000);
    assert(QF_GET_BLOCK_SIZE(p) == 1008);
    assert(qf->n_blocks == 1);

    qf_free_block(qf, p, QF_GET_BLOCK_SIZE(p));
    assert(qf->n_blocks == 2);
    assert(rbt_size(qf->large_blocks) == 2);

    assert(!qf_allot_block(qf, 20000));
    qf_free(qf);
    free((void *)region);
}

void
test_small_alloc() {
    size_t size = 10 * 1024;
    ptr region = (ptr)malloc(size);
    quick_fit *qf = qf_init(region, size);

    ptr p = qf_allot_block(qf, 16);
    assert(QF_GET_BLOCK_SIZE(p) == 16);
    assert(rbt_size(qf->large_blocks) == 1);
    assert(qf->buckets[1]->used == 63);
    for (size_t i = 0; i < 63; i++) {
        qf_allot_block(qf, 16);
        assert(qf->buckets[1]->used == 63 - i - 1);
    }
    assert(qf->free_space == 9 * 1024);

    qf_free(qf);
    free((void *)region);
}

void
test_random_allocs() {
    size_t size = 10 * 1024;
    ptr region = (ptr)malloc(size);
    quick_fit *qf = qf_init(region, size);

    for (size_t i = 0; i < 100; i++) {
        qf_allot_block(qf, (rand_n(20) + 1) * 16);
    }

    qf_print(qf);

    qf_free(qf);
    free((void *)region);
}

void
test_largest_free_block() {

    assert(QF_LARGE_BLOCK_SIZE(128) == 1024);

    size_t size = 1024;
    ptr region = (ptr)malloc(size);
    quick_fit *qf = qf_init(region, size);

    assert(qf_largest_free_block(qf) == 1024);

    qf_allot_block(qf, 128);
    assert(rbt_size(qf->large_blocks) == 0);
    assert(qf_largest_free_block(qf) == 128);

    for (int i = 0; i < 7; i++) {
        assert(qf_allot_block(qf, 128));
    }
    assert(qf_largest_free_block(qf) == 0);
    qf_free(qf);
    free((void *)region);
}

void
test_can_allot_p() {
    size_t size = 4096;
    ptr region = (ptr)malloc(size);
    quick_fit *qf = qf_init(region, size);

    assert(!qf_can_allot_p(qf, 4097));
    assert(qf_can_allot_p(qf, 4096));

    qf_allot_block(qf, 1024);
    qf_allot_block(qf, 1024);
    qf_allot_block(qf, 1024);

    // This is an interesting effect of bucket seeding.
    assert(qf_can_allot_p(qf, 1024));
    assert(!qf_allot_block(qf, 480));
    assert(!qf_can_allot_p(qf, 480));

    qf_free(qf);
    free((void *)region);
}

void
test_can_allot_p_random() {
    size_t size = 4096;
    ptr region = (ptr)malloc(size);
    quick_fit *qf = qf_init(region, size);

    for (int i = 0; i < 1000; i++) {
        size_t s = rand_n(1000) + 10;
        if (qf_can_allot_p(qf, s)) {
            assert(qf_allot_block(qf, s));
        } else {
            qf_clear(qf);
        }
    }
    qf_free(qf);
    free((void *)region);
}

void
test_bad_sizes() {
    size_t size = 4096;
    ptr region = (ptr)malloc(size);
    quick_fit *qf = qf_init(region, size);

    qf_allot_block(qf, 17);

    assert(qf->free_space == 4064);
    assert(qf_largest_free_block(qf) == 3072);

    qf_free(qf);
    free((void *)region);
}

void
test_fragmented_heap() {
    size_t size = 1024;
    ptr region = (ptr)malloc(size);
    quick_fit *qf = qf_init(region, size);

    qf_allot_block(qf, 128);
    assert(qf_can_allot_p(qf, 128));
    for (size_t i = 0; i < 7; i++) {
        assert(qf_allot_block(qf, 128));
    }
    qf_free(qf);
    free((void *)region);
}

void
test_largest_free_block2() {
    size_t size = 4096;
    ptr region = (ptr)malloc(size);
    quick_fit *qf = qf_init(region, size);

    qf_allot_block(qf, 1024);
    assert(qf_largest_free_block(qf) == 3072);
    ptr p = qf_allot_block(qf, 1024);
    assert(p);
    assert(qf_largest_free_block(qf) == 2048);
    qf_free_block(qf, p, QF_GET_BLOCK_SIZE(p));
    assert(qf_largest_free_block(qf) == 2048);
    qf_free(qf);
    free((void *)region);
}

void
test_can_allot_p2() {
    size_t size = 4096;
    ptr region = (ptr)malloc(size);
    quick_fit *qf = qf_init(region, size);

    qf_allot_block(qf, 3232);
    assert(qf_largest_free_block(qf) == 864);
    assert(qf_can_allot_p(qf, 864));
    assert(864 / QF_DATA_ALIGNMENT >= QF_N_BUCKETS);

    qf_free(qf);
    free((void *)region);
}

void
test_clear() {
    size_t size = 4096;
    ptr region = (ptr)malloc(size);
    quick_fit *qf = qf_init(region, size);

    assert(rbt_size(qf->large_blocks) == 1);
    qf_clear(qf);
    assert(rbt_size(qf->large_blocks) == 0);

    qf_free(qf);
    free((void *)region);
}

int
main(int argc, char *argv[]) {
    rand_init(0);
    PRINT_RUN(test_clear);
    PRINT_RUN(test_largest_free_block2);
    PRINT_RUN(test_fragmented_heap);
    PRINT_RUN(test_bad_sizes);
    PRINT_RUN(test_largest_free_block);
    PRINT_RUN(test_basic);
    PRINT_RUN(test_small_alloc);
    PRINT_RUN(test_random_allocs);
    PRINT_RUN(test_can_allot_p);
    PRINT_RUN(test_can_allot_p2);
    PRINT_RUN(test_can_allot_p_random);
    return 0;
}
