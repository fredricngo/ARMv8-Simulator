/*
 * CMSC 22200, Fall 2016
 *
 * ARM pipeline timing simulator
 *
 */
#ifndef _CACHE_H_
#define _CACHE_H_

#include <stdint.h>

typedef struct cache_line {
    int valid;
    uint64_t tag;
    uint64_t lru;
} cache_line_t;

typedef struct cache_set {
    cache_line_t *lines;
} cache_set_t;

typedef struct
{
    int num_sets;
    int num_ways;
    int block_size;
    cache_set_t *sets;

    int block_offset_bits;
    int set_index_bits;
    int tag_bits;
} cache_t;

cache_t *cache_new(int sets, int ways, int block);
void cache_destroy(cache_t *c);
int cache_update(cache_t *c, uint64_t addr);

#endif
