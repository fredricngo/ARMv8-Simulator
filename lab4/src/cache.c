/*
 * CMSC 22200, Fall 2016
 *
 * ARM pipeline timing simulator
 *
 */

#include "cache.h"
#include <stdlib.h>
#include <stdio.h>


cache_t *cache_new(int sets, int ways, int block)
{
    cache_t *cache = (cache_t *) malloc(sizeof(cache_t));
    if (!cache) {
        fprintf(stderr, "Failed to allocate memory for cache\n");
        exit(1);
    }
    for (int i = 0; i < sets; i++) {
        for (int j = 0; j < ways; j++) {
            cache->sets[i].lines = (cache_line_t *) malloc(sizeof(cache_line_t));
            if (!cache->sets[i].lines) {
                fprintf(stderr, "Failed to allocate memory for cache lines\n");
                for (int j = 0; j < i; j++) {
                free(cache->sets[j].lines);
                }
                free(cache->sets);
                free(cache);
                exit(1);
            }
        }
    }

    cache->num_sets = sets;
    cache->num_ways = ways;
    cache->block_size = block;

    cache->block_offset_bits = (int)log2(block);
    cache->set_index_bits = (int)log2(sets);
    cache->tag_bits = 64 - cache->set_index_bits - cache->block_offset_bits;

    for (int j = 0; j < ways; j++) {
        cache->sets[i].lines[j].valid = 0;
        cache->sets[i].lines[j].tag = 0;
        cache->sets[i].lines[j].lru = 0;
    }
    return cache;
}


void cache_destroy(cache_t *c)
{
    if (!c) return;
    for (int i = 0; i < c->num_sets; i++) {
        if (c->sets[i].lines) {
            free(c->sets[i].lines);
        }
    }
    if (c->sets) {
        free(c->sets);
    }
    free(c);

}


int cache_update(cache_t *c, uint64_t addr)
{
    if (!c) return 0;
    uint64_t block_offset_mask = (1ULL << c->block_offset_bits) - 1;
    uint64_t set_index_mask = (1ULL << c->set_index_bits) - 1;

    uint64_t block_offset = addr & block_offset_mask;
    uint64_t set_index = (addr >> c->block_offset_bits) & set_index_mask;
    uint64_t tag = addr >> (c->block_offset_bits + c->set_index_bits);
    cache_set_t *set = &c->sets[set_index];
    for (int i = 0; i < c->num_ways; i++) {
        cache_line_t *line = &set->lines[i];
        if (line->valid && line->tag == tag) {
            line->lru = 0; // Mark as most recently used
            for (int j = 0; j < c->num_ways; j++) {
                if (j != i) {
                    set->lines[j].lru++;
                }
            }
            return 1;
        }
    }
    int replace_index = 0;
    uint64_t oldest_lru = set->lines[0].lru_count;
    for (int i = 0; i < c->num_ways; i++) {
        // Prefer invalid lines first
        if (!set->lines[i].valid) {
            replace_index = i;
            break;
        }
        // Otherwise find the LRU line
        if (set->lines[i].lru_count < oldest_lru) {
            oldest_lru = set->lines[i].lru_count;
            replace_index = i;
        }
    }

    // Replace the selected line
    set->lines[replace_index].valid = 1;
    set->lines[replace_index].tag = tag;
    set->lines[replace_index].lru_count = c->global_lru_counter++;


}

