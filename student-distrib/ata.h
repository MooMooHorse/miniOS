/* ATA driver header file */
#ifndef _ATA_H
#define _ATA_H

#include "types.h"

extern int32_t identify_command();
extern int32_t detect_devtype (int32_t slavebit);
extern void    test_read_write();
extern void    dump_fs();
extern void    read_fs(int32_t slave_bit,uint32_t st,uint32_t ed);
#endif 

