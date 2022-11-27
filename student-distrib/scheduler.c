/*!
 * @brief This file contains implementations for scheduler.
 */
#include "scheduler.h"
#include "terminal.h"

/*!
 * @brief This function finds a new process and switch execution to it.
 * @param None.
 * @return None.
 * @sideeffect Swaps kernel stacks. Saves & Restores callee-saved registers.
 */
void
scheduler(void) {
    pcb_t* p;
    pcb_t* cur;
    uint32_t pid = get_pid();
    int32_t i;

    if (0 == pid) { return; }  // Wait until the shell is spawned.

    // Update status of process in the current terminal.
    cur = PCB(pid);
    terminal[terminal_index].pid = pid;
    cur->state = RUNNABLE;

    if (-1 == terminal_update(cur->terminal)) {
        printf("scheduler: terminal fatal error\n");
        while (1);
    }

    for (i = 1; i <= PCB_MAX; ++i) {
        p = PCB((pid - 1 + i) % PCB_MAX + 1);
        if (RUNNABLE == p->state) {
            break;
        }
    }

    pid = (pid - 1 + i) % PCB_MAX + 1;  // Commit update of `pid`.

    if (-1 == prog_video_update((PCB(pid))->terminal)) {
        printf("scheduler : terminal fatal error\n");
        while (1);
    }

    p->state = RUNNING;

    // Change user page table.
    if (0 != uvmmap_ext((pid - 1) * PROG_SIZE + PCB_BASE)
        || 0 != uvmremap_vid(pid)) {
        return;  // Failed to set up user memory.
    }
    setup_tss(pid);
    swtch(&cur->context, p->context);
}
