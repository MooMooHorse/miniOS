# Writable Filesystem

* This document is for our group to understand what I did for my first extra credit - writable file system



## Enable inode map and dblock map

* `read_dentry_by_index` in init for inode map
* `mark_dblock_by_inode`  for dblock map

* With two maps, we can allocate free datablocks



## `touch` program

* `read_dentry_addr_by_index()`
* use a loop to check the first empty slot

## `mv` program

* `read_dentry_addr_by_name()`  

## `rm` program

* `delete_file()`



## Write System Call : `append` program

* given inode, offset, buffer, length
* allocate space and 
* `write_data`

```
D:\vm\ece391\qemu_win\qemu-system-i386w.exe -hda "D:\vm\ece391\ece391_share\work\vm\devel.qcow" -m 512 -name devel -redir tcp:2022::22
```

