//========================================================//
//  cache.c                                               //
//  Source file for the Cache Simulator                   //
//                                                        //
//  Implement the I-cache, D-Cache and L2-cache as        //
//  described in the README                               //
//========================================================//

#include "cache.h"

//
// TODO:Student Information
//
const char *studentName = "Siddharth Thinakaran";
const char *studentID   = "A59005429";
const char *email       = "sthinakaran@ucsd.edu";

//------------------------------------//
//        Cache Configuration         //
//------------------------------------//

uint32_t icacheSets;     // Number of sets in the I$
uint32_t icacheAssoc;    // Associativity of the I$
uint32_t icacheHitTime;  // Hit Time of the I$

uint32_t dcacheSets;     // Number of sets in the D$
uint32_t dcacheAssoc;    // Associativity of the D$
uint32_t dcacheHitTime;  // Hit Time of the D$

uint32_t l2cacheSets;    // Number of sets in the L2$
uint32_t l2cacheAssoc;   // Associativity of the L2$
uint32_t l2cacheHitTime; // Hit Time of the L2$
uint32_t inclusive;      // Indicates if the L2 is inclusive

uint32_t blocksize;      // Block/Line size
uint32_t memspeed;       // Latency of Main Memory

//------------------------------------//
//          Cache Statistics          //
//------------------------------------//

uint64_t icacheRefs;       // I$ references
uint64_t icacheMisses;     // I$ misses
uint64_t icachePenalties;  // I$ penalties

uint64_t dcacheRefs;       // D$ references
uint64_t dcacheMisses;     // D$ misses
uint64_t dcachePenalties;  // D$ penalties

uint64_t l2cacheRefs;      // L2$ references
uint64_t l2cacheMisses;    // L2$ misses
uint64_t l2cachePenalties; // L2$ penalties

//------------------------------------//
//        Cache Data Structures       //
//------------------------------------//

//
//TODO: Add your Cache data structures here
//



uint32_t blockBits;
uint32_t i_indexBits;
uint32_t d_indexBits;
uint32_t l2_indexBits;
uint32_t *itag_array;
uint32_t *ivalid_array;
uint32_t *ilru_queue;
uint32_t *dtag_array;
uint32_t *dvalid_array;
uint32_t *dlru_queue;
uint32_t *l2tag_array;
uint32_t *l2valid_array;
uint32_t *l2lru_queue;
//------------------------------------//
//          Cache Functions           //
//------------------------------------//

// Initialize the Cache Hierarchy
//
void
init_cache()
{
  // Initialize cache stats
  icacheRefs        = 0;
  icacheMisses      = 0;
  icachePenalties   = 0;
  dcacheRefs        = 0;
  dcacheMisses      = 0;
  dcachePenalties   = 0;
  l2cacheRefs       = 0;
  l2cacheMisses     = 0;
  l2cachePenalties  = 0;
  
  //
  //TODO: Initialize Cache Simulator Data Structures
  //

  itag_array = (uint32_t*)malloc( icacheSets * icacheAssoc * sizeof(uint32_t));
  dtag_array = (uint32_t*)malloc( dcacheSets * dcacheAssoc * sizeof(uint32_t));
  l2tag_array = (uint32_t*)malloc( l2cacheSets * l2cacheAssoc * sizeof(uint32_t));
  ivalid_array = (uint32_t*)malloc( icacheSets * icacheAssoc * sizeof(uint32_t));
  dvalid_array = (uint32_t*)malloc( dcacheSets * dcacheAssoc * sizeof(uint32_t));
  l2valid_array = (uint32_t*)malloc( l2cacheSets * l2cacheAssoc * sizeof(uint32_t));
  ilru_queue = (uint32_t*)malloc( icacheSets * icacheAssoc * sizeof(uint32_t));
  dlru_queue = (uint32_t*)malloc( dcacheSets * dcacheAssoc * sizeof(uint32_t));
  l2lru_queue = (uint32_t*)malloc( l2cacheSets * l2cacheAssoc * sizeof(uint32_t));
  for(int i=0; i<icacheSets * icacheAssoc ; i++){
    itag_array[i]=0;
    ivalid_array[i]=0;
    ilru_queue[i]=0;
  }
  for(int i=0; i<icacheSets * icacheAssoc ; i++){
    dtag_array[i]=0;
    dvalid_array[i]=0;
    dlru_queue[i]=0;
  }
  for(int i=0; i< l2cacheSets * l2cacheAssoc ; i++){
    l2tag_array[i]=0;
    l2valid_array[i]=0;
    l2lru_queue[i]=0;
  }
}

// Perform a memory access through the icache interface for the address 'addr'
// Return the access time for the memory operation
//
uint32_t
icache_access(uint32_t addr)
{
  //
  //TODO: Implement I$
  //
  icacheRefs++;
  
  uint32_t index = ((addr/blocksize) & (icacheSets -1)); 
  uint32_t tag = addr/(blocksize * icacheSets);
  for(int i=0; i<icacheAssoc ; i++){
    if(itag_array[index*icacheAssoc+i]==tag & ivalid_array[index*icacheAssoc+i]==1){  //cache hit
        for(int j=0; j<icacheAssoc; j++){ 
          if(ilru_queue[index*icacheAssoc+i]<ilru_queue[index*icacheAssoc+j]){  //slots accessed more recent than this reduce priority by 1
            ilru_queue[index*icacheAssoc+j]--;
          }
        }
        ilru_queue[index*icacheAssoc+i]=icacheAssoc;  //set this one to be the most recently used
        return icacheHitTime;
    }
  }

  //cache miss, kick lru out of cache. no need to search non-valids, non-valids have 0 in their lru slot and are autopreferred
  // LRU queue eg: 4-way. For one set, initial = [0,0,0,0]  , 1st access [4,0,0,0] , 2nd access [3,4,0,0] ... [1,2,3,4]
  // If ilru_queue : [1,4,2,3] and idx 3 is accessed, ilru_queue = [1,3,4,2]
  int min_idx=0;
  for(int i=1; i<icacheAssoc; i++){   //find index with lowest value, i.e. LRU
    if(ilru_queue[index*icacheAssoc+i] < ilru_queue[index*icacheAssoc+min_idx]){  
      min_idx = i;
    }
  }

  itag_array[index*icacheAssoc+min_idx]=tag;  //replace the cache at that location
  ivalid_array[index*icacheAssoc+min_idx]=1;  //set valid

  for(int j=0; j<icacheAssoc; j++){  //Reduce priority of other elements of set
    if(ilru_queue[index*icacheAssoc+min_idx]<ilru_queue[index*icacheAssoc+j]){ 
      ilru_queue[index*icacheAssoc+j]--;
    }
  }
  ilru_queue[index*icacheAssoc+min_idx]=icacheAssoc;  //set newly replaced cache to be most recent
  
  icacheMisses++;
  int penalty = l2cache_access(addr);
  icachePenalties += penalty;
  return (icacheHitTime+penalty);
}

// Perform a memory access through the dcache interface for the address 'addr'
// Return the access time for the memory operation
//
uint32_t
dcache_access(uint32_t addr)
{
  //
  //TODO: Implement D$
  //
    dcacheRefs++;
  
  uint32_t index = ((addr/blocksize) & (dcacheSets -1)); 
  uint32_t tag = addr/(blocksize * dcacheSets);
  for(int i=0; i<dcacheAssoc ; i++){
    if(dtag_array[index*dcacheAssoc+i]==tag & dvalid_array[index*dcacheAssoc+i]==1){  //cache hit
        for(int j=0; j<dcacheAssoc; j++){ 
          if(dlru_queue[index*dcacheAssoc+i]<dlru_queue[index*dcacheAssoc+j]){  //slots accessed more recent than this reduce priority by 1
            dlru_queue[index*dcacheAssoc+j]--;
          }
        }
        dlru_queue[index*dcacheAssoc+i]=dcacheAssoc;  //set this one to be the most recently used
        return dcacheHitTime;
    }
  }

  //cache miss, kick lru out of cache. no need to search non-valids, non-valids have 0 in their lru slot and are autopreferred
  // LRU queue eg: 4-way. For one set, initial = [0,0,0,0]  , 1st access [4,0,0,0] , 2nd access [3,4,0,0] ... [1,2,3,4]
  // If ilru_queue : [1,4,2,3] and idx 3 is accessed, ilru_queue = [1,3,4,2]
  int min_idx=0;
  for(int i=1; i<dcacheAssoc; i++){   //find index with lowest value, i.e. LRU
    if(dlru_queue[index*dcacheAssoc+i] < dlru_queue[index*dcacheAssoc+min_idx]){  
      min_idx = i;
    }
  }

  dtag_array[index*dcacheAssoc+min_idx]=tag;  //replace the cache at that location
  dvalid_array[index*dcacheAssoc+min_idx]=1;  //set valid

  for(int j=0; j<dcacheAssoc; j++){  //Reduce priority of other elements of set
    if(dlru_queue[index*dcacheAssoc+min_idx]<dlru_queue[index*dcacheAssoc+j]){ 
      dlru_queue[index*dcacheAssoc+j]--;
    }
  }
  dlru_queue[index*dcacheAssoc+min_idx]=dcacheAssoc;  //set newly replaced cache to be most recent
  
  dcacheMisses++;
  int penalty = l2cache_access(addr);
  dcachePenalties += penalty;
  return (dcacheHitTime+penalty);
}

// Perform a memory access to the l2cache for the address 'addr'
// Return the access time for the memory operation
//
uint32_t
l2cache_access(uint32_t addr)
{
  //
  //TODO: Implement L2$
  //
  l2cacheRefs++;
  
  uint32_t index = ((addr/blocksize) & (l2cacheSets -1)); 
  uint32_t tag = addr/(blocksize * l2cacheSets);
  for(int i=0; i<l2cacheAssoc ; i++){
    if(l2tag_array[index*l2cacheAssoc+i]==tag & l2valid_array[index*l2cacheAssoc+i]==1){  //cache hit
        for(int j=0; j<l2cacheAssoc; j++){ 
          if(l2lru_queue[index*l2cacheAssoc+i]<l2lru_queue[index*l2cacheAssoc+j]){  //slots accessed more recent than this reduce priority by 1
            l2lru_queue[index*l2cacheAssoc+j]--;
          }
        }
        l2lru_queue[index*l2cacheAssoc+i]=l2cacheAssoc;  //set this one to be the most recently used
        return l2cacheHitTime;
    }
  }

  //cache miss, kick lru out of cache. no need to search non-valids, non-valids have 0 in their lru slot and are autopreferred
  // LRU queue eg: 4-way. For one set, initial = [0,0,0,0]  , 1st access [4,0,0,0] , 2nd access [3,4,0,0] ... [1,2,3,4]
  // If ilru_queue : [1,4,2,3] and idx 3 is accessed, ilru_queue = [1,3,4,2]
  int min_idx=0;
  for(int i=1; i<l2cacheAssoc; i++){   //find index with lowest value, i.e. LRU
    if(l2lru_queue[index*l2cacheAssoc+i] < l2lru_queue[index*l2cacheAssoc+min_idx]){  
      min_idx = i;
    }
  }

  l2tag_array[index*l2cacheAssoc+min_idx]=tag;  //replace the cache at that location
  l2valid_array[index*l2cacheAssoc+min_idx]=1;  //set valid

  for(int j=0; j<l2cacheAssoc; j++){  //Reduce priority of other elements of set
    if(l2lru_queue[index*l2cacheAssoc+min_idx]<l2lru_queue[index*l2cacheAssoc+j]){ 
      l2lru_queue[index*l2cacheAssoc+j]--;
    }
  }
  l2lru_queue[index*l2cacheAssoc+min_idx]=l2cacheAssoc;  //set newly replaced cache to be most recent
  
  l2cacheMisses++;
  l2cachePenalties += 100;
  return (l2cacheHitTime+100);
}
