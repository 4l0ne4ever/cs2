#ifndef INDEX_H
#define INDEX_H

#include "types.h"

// Hash table for fast lookups
#define HASH_TABLE_SIZE 10007

typedef struct IndexNode
{
    int key;
    off_t file_offset;
    struct IndexNode *next;
} IndexNode;

typedef struct
{
    IndexNode *buckets[HASH_TABLE_SIZE];
    pthread_rwlock_t lock;
} Index;

// Initialize index
void index_init(Index *idx);

// Add entry
void index_add(Index *idx, int key, off_t offset);

// Lookup entry
off_t index_lookup(Index *idx, int key);

// Remove entry
void index_remove(Index *idx, int key);

// Hash function
unsigned int hash(int key);

// Load index at startup
void load_indexes();

#endif // INDEX_H
