
#include <inc/riscv/lib.h>


void
umain(int argc, char **argv)
{
    char cur[4];

    cur[0] = 'a';
    if (fork() == 0) {
      cur[0] = 'b';
    }

    cprintf("%04x: I am '%s'\n", sys_getenvid(), cur);
}
