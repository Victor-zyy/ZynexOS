#include <kern/riscv/clint.h>
#include <inc/riscv/sbi.h>
#include <inc/riscv/stdio.h>
#include <kern/riscv/env.h>

/* In this file, we are gonna to initialize the timer in clint,
 * so that we can make the preemptive multitasking done.
 * And, differs from the lapic of each rtc to cpus in SMP, in riscv
 * there only resides one clint peripheral in the soc. So we have
 * to handle somthing maybe 
 */

/* read mtime register to get curent time cycles */
static unsigned long get_ticks()
{
	unsigned long n;
	__asm__ __volatile__("rdtime %0" : "=r"(n));
	return n;
}
void clint_init(void){
  
  /* set mtimecmp for about 1s + mtime */
  sbi_set_time(get_ticks() + TIME_TENMILISECOND_TICKS);
  /* enable interrupt */
  csr_set(CSR_SIE, MIP_STIP);
  /* enable global interrupt */
  csr_set(CSR_SSTATUS, SSTATUS_SIE);

}

void reset_timer(void){

  static int i = 0;
  //cprintf("curenv->id 0x%08lx sepc 0x%08lx getting into timer times : %d\n", curenv->env_id, curenv->env_tf.sepc, i++);
  /* set mtimecmp for about 1s + mtime */
  sbi_set_time(get_ticks() + TIME_TENMILISECOND_TICKS);
  /* enable interrupt */
  csr_set(CSR_SIE, MIP_STIP);
}
