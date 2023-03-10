#include "ece391sysnum.h"

/* 
 * Rather than create a case for each number of arguments, we simplify
 * and use one macro for up to three arguments; the system calls should
 * ignore the other registers, and they're caller-saved anyway.
 */
#define DO_CALL(name,number)   \
.GLOBL name                   ;\
name:   PUSHL	%EBX          ;\
	MOVL	$number,%EAX  ;\
	MOVL	8(%ESP),%EBX  ;\
	MOVL	12(%ESP),%ECX ;\
	MOVL	16(%ESP),%EDX ;\
	INT	$0x80         ;\
	POPL	%EBX          ;\
	RET

/* the system call library wrappers */
DO_CALL(ece391_halt,SYS_HALT)
DO_CALL(ece391_execute,SYS_EXECUTE)
DO_CALL(ece391_read,SYS_READ)
DO_CALL(ece391_write,SYS_WRITE)
DO_CALL(ece391_open,SYS_OPEN)
DO_CALL(ece391_close,SYS_CLOSE)
DO_CALL(ece391_getargs,SYS_GETARGS)
DO_CALL(ece391_vidmap,SYS_VIDMAP)
DO_CALL(ece391_set_handler,SYS_SET_HANDLER)
DO_CALL(ece391_sigreturn,SYS_SIGRETURN)
DO_CALL(ece391_set_cursor,SYS_SET_CURSOR)
DO_CALL(ece391_get_cursor,SYS_GET_CURSOR)
DO_CALL(ece391_file_create,SYS_FILE_CREATE)
DO_CALL(ece391_file_remove,SYS_FILE_REMOVE)
DO_CALL(ece391_file_rename,SYS_FILE_RENAME)
DO_CALL(ece391_getc,SYS_GETC)
DO_CALL(ece391_sb16_ioctl,SYS_SB16_IOCTL)
DO_CALL(ece391_kmalloc_demo,SYS_KMALLOC_DEMO)
DO_CALL(ece391_buddy_traverse,SYS_BUDDY_TRAVERSE)

/* Call the main() function, then halt with its return value. */

.GLOBAL _start
_start:
	CALL	main
    PUSHL   $0
    PUSHL   $0
	PUSHL	%EAX
	CALL	ece391_halt

