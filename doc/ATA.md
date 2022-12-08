# ATA

* This document is to store all the memory-related info I learnt at 2022-12-05





## Buddy Allocator vs. Red Black Tree (Allocate and GC)

* All memory management **after** pages have been allocated from slabs

### Differences

* Buddy Allocator uses bitmap for each layer, so the spatial overhead is $O(\log N)$ where N is the number of pages to be allocated
* Red Black tree has more metadata
* The Time complexity and GC complexity is all $O(\log N)$

## Flash Drive, Hard Disk, Virtual Memory, and Process

* FTL Flash Translation Layer has the same functionality as Paging scheme
  * LBA -> Flash address
* Flash map is to unify the **layers** so GC and **a portion of paging (PTE)** are shared

### DRAM SRAM and NVM

* All memory
* unit in ns instead of SSD in ms



## Consistency vs. Persistency 

> https://en.wikipedia.org/wiki/Persistence_(computer_science) - persistency

Consistency - Are data updated for multi-threaded/unit-threaded process within/without distributed system



## 
