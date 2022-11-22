#include "scheduler.h"

void
scheduler(void) {
    pcb_t* p;
    pcb_t* t;
    uint32_t pid = get_pid();
    int32_t i;

    if (0 == pid) { return; }  // Wait until the shell is spawned.

    // Update status of process in the current terminal.
    terminal[terminal_index].pid = pid;
    ((pcb_t*) cur_proc)->state = RUNNABLE;

    for (i = 1; i <= MAX_TERMINAL_NUM; ++i) {
        pid = terminal[(terminal_index + i) % MAX_TERMINAL_NUM].pid;
        if (0 == pid) {
            continue;   // No active process in this terminal.
        }
        p = PCB(pid);
        if (RUNNABLE == p->state) {
            break;
        }
    }

    printf("context switch: #%u --> #%u\n", get_pid(), pid);

    t = (pcb_t*) cur_proc;
    cur_proc = (uint32_t) p;
    p = t;
    ((pcb_t*) cur_proc)->state = RUNNING;

    // Switch to user page table.
    if (0 != uvmmap_ext((pid - 1) * PROG_SIZE + PCB_BASE)) {
        return;  // Failed to map user memory.
    }
    setup_tss(pid);
    swtch(&p->context, ((pcb_t*) cur_proc)->context);
}
