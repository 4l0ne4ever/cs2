// test_index.c - Index System Tests

#include "../include/index.h"
#include <stdio.h>
#include <assert.h>

void test_hash_function()
{
    printf("Testing hash function...\n");

    unsigned int h1 = hash(1);
    unsigned int h2 = hash(2);
    unsigned int h100 = hash(100);

    assert(h1 < HASH_TABLE_SIZE);
    assert(h2 < HASH_TABLE_SIZE);
    assert(h100 < HASH_TABLE_SIZE);

    printf("✅ hash function passed\n");
}

void test_index_add_lookup()
{
    printf("\nTesting index_add and index_lookup...\n");

    Index idx;
    index_init(&idx);

    // Add entries
    index_add(&idx, 1, 100);
    index_add(&idx, 2, 200);
    index_add(&idx, 3, 300);

    // Lookup
    off_t offset1 = index_lookup(&idx, 1);
    off_t offset2 = index_lookup(&idx, 2);
    off_t offset3 = index_lookup(&idx, 3);

    assert(offset1 == 100);
    assert(offset2 == 200);
    assert(offset3 == 300);

    printf("✅ index_add and index_lookup passed\n");
}

void test_index_remove()
{
    printf("\nTesting index_remove...\n");

    Index idx;
    index_init(&idx);

    index_add(&idx, 1, 100);
    index_add(&idx, 2, 200);

    index_remove(&idx, 1);

    off_t offset1 = index_lookup(&idx, 1);
    off_t offset2 = index_lookup(&idx, 2);

    assert(offset1 == -1);  // Not found
    assert(offset2 == 200); // Still exists

    printf("✅ index_remove passed\n");
}

int main()
{
    printf("=== Index Tests ===\n\n");

    test_hash_function();
    test_index_add_lookup();
    test_index_remove();

    printf("\n=== All index tests passed! ===\n");

    return 0;
}
