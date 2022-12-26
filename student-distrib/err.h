#ifndef _ERR_H

/* halt status : 0~255 : used as input to halt and user level return or exception */
#define STATUS_NORMAL    0   /* stipulated */
#define STATUS_EXCEPTION 250 /* self-defined */
/* used in execute system call */
/* errono : 32 bits integer */
#define ERR_NO_CMD  -1    /* stipulated */
#define ERR_EXCEPTION 256 /* stipulated */
#define ERR_FS_READ 257  /* self-defined */
#define ERR_BAD_PID 258  /* self-defined */
#define ERR_VM_FAILURE 259  /* self-defined */

#ifndef ASM
#include "types.h"
extern int32_t handle_error(uint32_t status);
#endif 

#endif

