///////////////////////////////////////////////////////////////////////////////
// You will need to modify this file to implement part A and, for extra      //
// credit, parts E and F.                                                    //
///////////////////////////////////////////////////////////////////////////////

// cache.cpp
// Defines the functions used to implement the cache.

#include "cache.h"
#include <stdio.h>
#include <stdlib.h>
#include <ctime>
#include <iostream>
#include "vector"   

using namespace std;
// You may add any other #include directives you need here, but make sure they
// compile on the reference machine!

///////////////////////////////////////////////////////////////////////////////
//                    EXTERNALLY DEFINED GLOBAL VARIABLES                    //
///////////////////////////////////////////////////////////////////////////////

/**
 * The current clock cycle number.
 * 
 * This can be used as a timestamp for implementing the LRU replacement policy.
 */
extern uint64_t current_cycle;

/**
 * For static way partitioning, the quota of ways in each set that can be
 * assigned to core 0.
 * 
 * The remaining number of ways is the quota for core 1.
 * 
 * This is used to implement extra credit part E.
 */
extern unsigned int SWP_CORE0_WAYS;

extern unsigned int NUM_CORES;

///////////////////////////////////////////////////////////////////////////////
//                           FUNCTION DEFINITIONS                            //
///////////////////////////////////////////////////////////////////////////////

// As described in cache.h, you are free to deviate from the suggested
// implementation as you see fit.

// The only restriction is that you must not remove cache_print_stats() or
// modify its output format, since its output will be used for grading.

/**
 * Allocate and initialize a cache.
 * 
 * This is intended to be implemented in part A.
 *
 * @param size The size of the cache in bytes.
 * @param associativity The associativity of the cache.
 * @param line_size The size of a cache line in bytes.
 * @param replacement_policy The replacement policy of the cache.
 * @return A pointer to the cache.
 */
Cache *cache_new(uint64_t size, uint64_t associativity, uint64_t line_size,
                 ReplacementPolicy replacement_policy)
{
    // TODO: Allocate memory to the data structures and initialize the required
    //       fields. (You might want to use calloc() for this.)
    uint64_t num_cache_lines = size/line_size;
    uint64_t num_ways = associativity;
    uint64_t num_sets = num_cache_lines/num_ways;
    // TODO: optimize the allocation maybe
    CacheSet *cache_sets = new CacheSet[num_sets];
    Cache *cache = new Cache;
    cache->cache_sets = cache_sets;
    cache->num_sets = num_sets;
    cache->line_size = line_size;
    cache->num_ways = num_ways;
    cache->replacement_policy = replacement_policy;
    cache->stat_dirty_evicts = 0;
    cache->stat_read_access = 0;
    cache->stat_read_miss = 0;
    cache->stat_write_access = 0;
    cache->stat_write_miss = 0;
    cache->last_partitioning_cycle = 0;
    cache->DWP_CORE0_WAYS = num_ways / 2;
    cache->umon = new CacheSet*[NUM_CORES];
    cache->umon_hit_counters = new uint64_t*[NUM_CORES];
    cache->umon_miss_counters = new uint64_t[NUM_CORES];
    for(uint i = 0; i < NUM_CORES; i++) {
        cache->umon[i] = new CacheSet[NUM_SAMPLED_SETS];
        cache->umon_hit_counters[i] = new uint64_t[num_ways];
    }
    #ifdef DEBUG
    for(uint32_t i = 0; i < cache->num_sets; i++) {
        for(uint32_t j = 0; j < cache->num_ways; j++) {
            CacheLine c = cache->cache_sets[i].cache_lines[j];
            cout << c.core_id << " " << c.tag << " " << c.valid << " " << c.dirty << " " << "\n";
        }
    }
    #endif
    return cache;
}

/**
 * Access the cache at the given address.
 * 
 * Also update the cache statistics accordingly.
 * 
 * This is intended to be implemented in part A.
 * 
 * @param c The cache to access.
 * @param line_addr The address of the cache line to access (in units of the
 *                  cache line size, i.e., excluding the line offset bits).
 * @param is_write Whether this access is a write.
 * @param core_id The CPU core ID that requested this access.
 * @return Whether the cache access was a hit or a miss.
 */
CacheResult cache_access(Cache *c, uint64_t line_addr, bool is_write,
                         unsigned int core_id)
{
    // TODO: Return HIT if the access hits in the cache, and MISS otherwise.
    // TODO: If is_write is true, mark the resident line as dirty.
    // TODO: Update the appropriate cache statistics.
    if(c->replacement_policy == DWP) {
        umon_access(c, line_addr, core_id);
    }
    CacheResult hit = MISS;
    uint64_t index = line_addr % c->num_sets;
    uint64_t tag = line_addr / c->num_sets;
    CacheSet *set = &(c->cache_sets[index]);
    for(uint64_t i = 0; i < c->num_ways; i++) {
        if(set->cache_lines[i].valid && set->cache_lines[i].tag == tag && set->cache_lines[i].core_id == core_id) {
            if(is_write) {
                set->cache_lines[i].dirty = is_write;
            }
            set->cache_lines[i].last_access_time = current_cycle;
            hit = HIT;
            break;
        }
    }
    c->stat_read_access += !is_write;
    c->stat_write_access += is_write;
    c->stat_read_miss += !hit && !is_write;
    c->stat_write_miss += !hit && is_write;
    #ifdef DEBUG
    if(!hit) {
        printf("MISS: %ld\n", current_cycle);
    }
    #endif
    return (CacheResult)hit;
}

/**
 * Install the cache line with the given address.
 * 
 * Also update the cache statistics accordingly.
 * 
 * This is intended to be implemented in part A.
 * 
 * @param c The cache to install the line into.
 * @param line_addr The address of the cache line to install (in units of the
 *                  cache line size, i.e., excluding the line offset bits).
 * @param is_write Whether this install is triggered by a write.
 * @param core_id The CPU core ID that requested this access.
 */
void cache_install(Cache *c, uint64_t line_addr, bool is_write,
                   unsigned int core_id)
{
    // TODO: Use cache_find_victim() to determine the victim line to evict.
    // TODO: Copy it into a last_evicted_line field in the cache in order to
    //       track writebacks.
    // TODO: Initialize the victim entry with the line to install.
    // TODO: Update the appropriate cache statistics.
    uint64_t set_index = line_addr % c->num_sets;
    uint64_t tag = line_addr / c->num_sets;
    uint way_index = cache_find_victim(c, set_index, core_id);
    CacheLine *cache_line = &(c->cache_sets[set_index].cache_lines[way_index]);
    c->last_evicted_line = *cache_line;
    if(cache_line->valid && cache_line->dirty) {
        c->stat_dirty_evicts++;
    }
    cache_line->core_id = core_id;
    cache_line->dirty = is_write;
    cache_line->valid = true;
    cache_line->tag = tag;
    cache_line->last_access_time = current_cycle;
    cache_line->line_addr = line_addr;
}

unsigned int random_number(Cache *c) {
    return rand() % c->num_ways;
}

unsigned int partitioning_way_index(Cache *c, unsigned int set_index, unsigned int core_id, int partition_ways) {
    CacheSet set = c->cache_sets[set_index];
    int way_index = 0;
    uint count_ways = 0;
    for(uint32_t i = 0; i < c->num_ways; i++) {
        if(!set.cache_lines[i].valid) {
            return i;
        }
        if(set.cache_lines[i].core_id == core_id) {
            count_ways++;
        }
    }
    uint threshold = partition_ways;
    if(core_id) threshold = c->num_ways - partition_ways;
    uint temp_core_id = core_id; 
    if(count_ways < threshold) temp_core_id = 1 - core_id;
    way_index = -1;
    for(uint64_t i = 0; i < c->num_ways; i++) {
        if(set.cache_lines[i].core_id == temp_core_id) {
            if(way_index == -1) {
                way_index = i;
            } else if(set.cache_lines[i].last_access_time < set.cache_lines[way_index].last_access_time) {
                way_index = i;
            }
        }
    }
    return way_index;
}

/**
 * Find which way in a given cache set to replace when a new cache line needs
 * to be installed. This should be chosen according to the cache's replacement
 * policy.
 * 
 * The returned victim can be valid (non-empty), in which case the calling
 * function is responsible for evicting the cache line from that victim way.
 * 
 * This is intended to be initially implemented in part A and, for extra
 * credit, extended in parts E and F.
 * 
 * @param c The cache to search.
 * @param set_index The index of the cache set to search.
 * @param core_id The CPU core ID that requested this access.
 * @return The index of the victim way.
 */
unsigned int cache_find_victim(Cache *c, unsigned int set_index,
                               unsigned int core_id)
{
    // TODO: Find a victim way in the given cache set according to the cache's
    //       replacement policy.
    // TODO: In part A, implement the LRU and random replacement policies.
    // TODO: In part E, for extra credit, implement static way partitioning.
    // TODO: In part F, for extra credit, implement dynamic way partitioning.
    if(c->replacement_policy == LRU) {
        #ifdef DEBUG
        printf("LRU policy\n");
        #endif
        CacheSet set = c->cache_sets[set_index];
        uint way_index = 0;
        for(uint64_t i = 1; i < c->num_ways; i++) {
            if(set.cache_lines[i].last_access_time < set.cache_lines[way_index].last_access_time) {
                way_index = i;
            }
        }
        return way_index;
    } else if(c->replacement_policy == RANDOM) {
        #ifdef DEBUG
        printf("random policy\n");
        #endif
        return random_number(c);
    } else if(c->replacement_policy == SWP) {
        return partitioning_way_index(c, set_index, core_id, SWP_CORE0_WAYS);
    } else if(c->replacement_policy == DWP) {
        if(current_cycle - c->last_partitioning_cycle >= PARTITION_INTERVAL) {
            cache_run_partitioning_alg(c);
            c->last_partitioning_cycle = current_cycle;
        }
        return partitioning_way_index(c, set_index, core_id, c->DWP_CORE0_WAYS);
    }
    return 0;
}

bool is_sampled_set(Cache *c, unsigned int set_index) {
    if(c->num_sets < NUM_SAMPLED_SETS) {
        return true;
    }
    uint32_t stride = c->num_sets/NUM_SAMPLED_SETS;
    return !(set_index % stride) && (stride*NUM_SAMPLED_SETS > set_index);
}

unsigned int get_sampled_set_index(Cache *c, unsigned int set_index) {
    uint32_t stride = c->num_sets / NUM_SAMPLED_SETS;
    return set_index / stride;
}


void umon_access(Cache *c, uint64_t line_addr, unsigned int core_id) {
    uint set_index = line_addr % c->num_sets;
    if(!is_sampled_set(c, set_index)) {
        return;
    }
    uint sampled_set_index = get_sampled_set_index(c, set_index);
    CacheSet *s = &(c->umon[core_id][sampled_set_index]);
    uint32_t tag = line_addr / c->num_sets;
    int lru_index = 0;
    int empty_index = -1;
    int hit_index = -1;
    for(uint i = 0; i < c->num_ways; i++) {
        if(!(s->cache_lines[i].valid)) {
            if(empty_index == -1) {
                empty_index = i;
            }
            continue;
        }
        if(s->cache_lines[i].tag == tag) {
            hit_index = i;
            break;
        }
        if(s->cache_lines[i].last_access_time < s->cache_lines[lru_index].last_access_time) {
            lru_index = i;
        }
    }
    if(hit_index != -1) {
        uint64_t hit_time = s->cache_lines[hit_index].last_access_time;
        unsigned int recency_pos = 0;
        for (unsigned int i = 0; i < c->num_ways; i++) {
            if (s->cache_lines[i].valid && s->cache_lines[i].last_access_time > hit_time) {
                recency_pos++;
            }
        }
        c->umon_hit_counters[core_id][recency_pos]++;
        s->cache_lines[hit_index].last_access_time = current_cycle;
        return;
    }
    c->umon_miss_counters[core_id]++;
    if(empty_index != -1) {
        s->cache_lines[empty_index] = {1, 0, tag, core_id, current_cycle, line_addr};
        return;
    }
    s->cache_lines[lru_index] = {1, 0, tag, core_id, current_cycle, line_addr};
}

void cache_run_partitioning_alg(Cache *c) {
    std::vector<std::vector<uint64_t>> misses_per_way(NUM_CORES, std::vector<uint64_t> (c->num_ways+1, 0));
 
    for (uint core = 0; core < NUM_CORES; core++) {
        misses_per_way[core][c->num_ways] = c->umon_miss_counters[core];

        for (int w = c->num_ways - 1; w >= 1; w--) {
            misses_per_way[core][w] = misses_per_way[core][w + 1] + c->umon_hit_counters[core][w];
        }
    }

    uint64_t min_total_misses = (uint64_t)-1;
    unsigned int best_partition = c->num_ways / 2;

    for (unsigned int core0_ways = 1; core0_ways < c->num_ways; core0_ways++) {
        unsigned int core1_ways = c->num_ways - core0_ways;

        uint64_t core0_misses = misses_per_way[0][core0_ways];
        uint64_t core1_misses = misses_per_way[1][core1_ways];
        uint64_t total_misses = core0_misses + core1_misses;

        if (total_misses < min_total_misses) {
            min_total_misses = total_misses;
            best_partition = core0_ways;
        }
    }

    c->DWP_CORE0_WAYS = best_partition;
    // printf("\ncore_0_ways:  %ld\n", c->DWP_CORE0_WAYS);

    for (uint core = 0; core < NUM_CORES; core++) {
        c->umon_miss_counters[core] /= 2;
        for (unsigned int w = 0; w < c->num_ways; w++) {
            c->umon_hit_counters[core][w] /= 2;
        }
    }
}


/**
 * Print the statistics of the given cache.
 * 
 * This is implemented for you. You must not modify its output format.
 * 
 * @param c The cache to print the statistics of.
 * @param label A label for the cache, which is used as a prefix for each
 *              statistic.
 */
void cache_print_stats(Cache *c, const char *header)
{
    double read_miss_percent = 0.0;
    double write_miss_percent = 0.0;

    if (c->stat_read_access)
    {
        read_miss_percent = 100.0 * (double)(c->stat_read_miss) /
                            (double)(c->stat_read_access);
    }

    if (c->stat_write_access)
    {
        write_miss_percent = 100.0 * (double)(c->stat_write_miss) /
                             (double)(c->stat_write_access);
    }

    printf("\n");
    printf("%s_READ_ACCESS     \t\t : %10llu\n", header, c->stat_read_access);
    printf("%s_WRITE_ACCESS    \t\t : %10llu\n", header, c->stat_write_access);
    printf("%s_READ_MISS       \t\t : %10llu\n", header, c->stat_read_miss);
    printf("%s_WRITE_MISS      \t\t : %10llu\n", header, c->stat_write_miss);
    printf("%s_READ_MISS_PERC  \t\t : %10.3f\n", header, read_miss_percent);
    printf("%s_WRITE_MISS_PERC \t\t : %10.3f\n", header, write_miss_percent);
    printf("%s_DIRTY_EVICTS    \t\t : %10llu\n", header, c->stat_dirty_evicts);
}
