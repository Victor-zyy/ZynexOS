#include <inc/riscv/lib.h>

#include <arch/thread.h>
#include <arch/threadq.h>
#include <arch/setjmp.h>

static thread_id_t max_tid;
static struct thread_context *cur_tc;

static struct thread_queue thread_queue;
static struct thread_queue kill_queue;

void
thread_init(void) {
    threadq_init(&thread_queue);
    max_tid = 0;
}

uint32_t
thread_id(void) {
    return cur_tc->tc_tid;
}

void
thread_wakeup(volatile uint32_t *addr) {
    struct thread_context *tc = thread_queue.tq_first;
    while (tc) {
	if (tc->tc_wait_addr == addr)
	    tc->tc_wakeup = 1;
	tc = tc->tc_queue_link;
    }
}

void
thread_wait(volatile uint32_t *addr, uint32_t val, uint32_t msec) {
    uint32_t s = sys_time_msec();
    uint32_t p = s;

    cur_tc->tc_wait_addr = addr;
    cur_tc->tc_wakeup = 0;

    while (p < msec) {
	if (p < s)
	    break;
	if (addr && *addr != val)
	    break;
	if (cur_tc->tc_wakeup)
	    break;

	thread_yield();
	p = sys_time_msec();
    }

    cur_tc->tc_wait_addr = 0;
    cur_tc->tc_wakeup = 0;
}

int
thread_wakeups_pending(void)
{
    struct thread_context *tc = thread_queue.tq_first;
    int n = 0;
    while (tc) {
	if (tc->tc_wakeup)
	    ++n;
	tc = tc->tc_queue_link;
    }
    return n;
}

int
thread_onhalt(void (*fun)(thread_id_t)) {
    if (cur_tc->tc_nonhalt >= THREAD_NUM_ONHALT)
	return -E_NO_MEM;

    cur_tc->tc_onhalt[cur_tc->tc_nonhalt++] = fun;
    return 0;
}

static thread_id_t
alloc_tid(void) {
    int tid = max_tid++;
    if (max_tid == (uint32_t)~0)
	panic("alloc_tid: no more thread ids");
    return tid;
}

static void
thread_set_name(struct thread_context *tc, const char *name)
{
    strncpy(tc->tc_name, name, name_size - 1);
    tc->tc_name[name_size - 1] = 0;
}

static void
thread_entry(void) {
    cur_tc->tc_entry(cur_tc->tc_arg);
    thread_halt();
}

int
thread_create(thread_id_t *tid, const char *name, 
		void (*entry)(uint64_t), uint64_t arg) {
    struct thread_context *tc = malloc(sizeof(struct thread_context));
    if (!tc)
	return -E_NO_MEM;

    memset(tc, 0, sizeof(struct thread_context));
    
    thread_set_name(tc, name);
    tc->tc_tid = alloc_tid();

    tc->tc_stack_bottom = malloc(stack_size);/* FIXME: 4K */
    if (!tc->tc_stack_bottom) {
	free(tc);
	return -E_NO_MEM;
    }

    void *stacktop = tc->tc_stack_bottom + stack_size;
    // Terminate stack unwinding
    stacktop = stacktop - 8;    /* FIXME: 4->8 */
    memset(stacktop, 0, 8);
    
    memset(&tc->tc_jb, 0, sizeof(tc->tc_jb));
    tc->tc_jb.jb_sp = (uint64_t)stacktop;
    tc->tc_jb.jb_ra = (uint64_t)&thread_entry;
    tc->tc_entry = entry;
    tc->tc_arg = arg;

    threadq_push(&thread_queue, tc);

    if (tid)
	*tid = tc->tc_tid;
    return 0;
}

static void
thread_clean(struct thread_context *tc) {
    if (!tc) return;

    int i;
    for (i = 0; i < tc->tc_nonhalt; i++)
	tc->tc_onhalt[i](tc->tc_tid);
    free(tc->tc_stack_bottom);
    free(tc);
}

void
thread_halt() {
    // right now the kill_queue will never be more than one
    // clean up a thread if one is on the queue
    thread_clean(threadq_pop(&kill_queue));

    threadq_push(&kill_queue, cur_tc);
    cur_tc = NULL;
    thread_yield();
    // WHAT IF THERE ARE NO MORE THREADS? HOW DO WE STOP?
    // when yield has no thread to run, it will return here!
    exit();
}

void
thread_yield(void) {
    struct thread_context *next_tc = threadq_pop(&thread_queue);

    if (!next_tc)
	return;

    if (cur_tc) {
	if (jos_setjmp(&cur_tc->tc_jb) != 0)
	    return;
	threadq_push(&thread_queue, cur_tc);
    }

    cur_tc = next_tc;
    jos_longjmp(&cur_tc->tc_jb, 1);
}

static void
print_jb(struct thread_context *tc) {
    cprintf("jump buffer for thread %s:\n", tc->tc_name);
    cprintf("\tra: %x\n", tc->tc_jb.jb_ra);
    cprintf("\tsp: %x\n", tc->tc_jb.jb_sp);
    cprintf("\ts0: %x\n", tc->tc_jb.jb_s0);
    cprintf("\ts1: %x\n", tc->tc_jb.jb_s1);
    cprintf("\ts2: %x\n", tc->tc_jb.jb_s2);
    cprintf("\ts3: %x\n", tc->tc_jb.jb_s3);
    cprintf("\ts4: %x\n", tc->tc_jb.jb_s4);
    cprintf("\ts5: %x\n", tc->tc_jb.jb_s5);
    cprintf("\ts6: %x\n", tc->tc_jb.jb_s6);
    cprintf("\ts7: %x\n", tc->tc_jb.jb_s7);
    cprintf("\ts8: %x\n", tc->tc_jb.jb_s8);
    cprintf("\ts9: %x\n", tc->tc_jb.jb_s9);
    cprintf("\ts10: %x\n", tc->tc_jb.jb_s10);
    cprintf("\ts11: %x\n", tc->tc_jb.jb_s11);
}
