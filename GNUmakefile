#
# This makefile system follows the structuring conventions
# recommended by Peter Miller in his excellent paper:
#
#	Recursive Make Considered Harmful
#	http://aegis.sourceforge.net/auug97.pdf
#
OBJDIR := obj
# need to be overiden when pass

ARCH ?= riscv

# Run 'make V=1' to turn on verbose commands, or 'make V=0' to turn them off.
ifeq ($(V),1)
override V =
endif
ifeq ($(V),0)
override V = @
endif

-include conf/lab.mk

-include conf/env.mk

LABSETUP ?= ./

TOP = .

# Cross-compiler jos toolchain
#
# This Makefile will automatically use the cross-compiler toolchain
# installed as 'i386-jos-elf-*', if one exists.  If the host tools ('gcc',
# 'objdump', and so forth) compile for a 32-bit x86 ELF target, that will
# be detected as well.  If you have the right compiler toolchain installed
# using a different name, set GCCPREFIX explicitly in conf/env.mk
ifeq ($(ARCH),riscv)
# try to infer the correct GCCPREFIX
GCCPREFIX =riscv64-linux-gnu-
#GCCPREFIX =/home/zyy/tools/riscv/bin/riscv64-unknown-linux-gnu-
else
GCCPREFIX =i386-jos-elf-
endif

ifndef GCCPREFIX
GCCPREFIX := $(shell if i386-jos-elf-objdump -i 2>&1 | grep '^elf32-i386$$' >/dev/null 2>&1; \
	then echo 'i386-jos-elf-'; \
	elif objdump -i 2>&1 | grep 'elf32-i386' >/dev/null 2>&1; \
	then echo ''; \
	else echo "***" 1>&2; \
	echo "*** Error: Couldn't find an i386-*-elf version of GCC/binutils." 1>&2; \
	echo "*** Is the directory with i386-jos-elf-gcc in your PATH?" 1>&2; \
	echo "*** If your i386-*-elf toolchain is installed with a command" 1>&2; \
	echo "*** prefix other than 'i386-jos-elf-', set your GCCPREFIX" 1>&2; \
	echo "*** environment variable to that prefix and run 'make' again." 1>&2; \
	echo "*** To turn off this error, run 'gmake GCCPREFIX= ...'." 1>&2; \
	echo "***" 1>&2; exit 1; fi)
endif

ifeq ($(ARCH),riscv)
# try to infer the correct QEMU
QEMU = /opt/riscv-qemu-7.0.0/bin/qemu-system-riscv64
else
QEMU = /opt/qemu_2.3.0/bin/qemu-system-i386
endif

ifndef QEMU
QEMU := $(shell if which qemu >/dev/null 2>&1; \
	then echo qemu; exit; \
        elif which qemu-system-i386 >/dev/null 2>&1; \
        then echo qemu-system-i386; exit; \
	else \
	qemu=/Applications/Q.app/Contents/MacOS/i386-softmmu.app/Contents/MacOS/i386-softmmu; \
	if test -x $$qemu; then echo $$qemu; exit; fi; fi; \
	echo "***" 1>&2; \
	echo "*** Error: Couldn't find a working QEMU executable." 1>&2; \
	echo "*** Is the directory containing the qemu binary in your PATH" 1>&2; \
	echo "*** or have you tried setting the QEMU variable in conf/env.mk?" 1>&2; \
	echo "***" 1>&2; exit 1)
endif

# try to generate a unique GDB port
GDBPORT	:= $(shell expr `id -u` % 5000 + 25000)

CC	:= $(GCCPREFIX)gcc -pipe
AS	:= $(GCCPREFIX)as
AR	:= $(GCCPREFIX)ar
LD	:= $(GCCPREFIX)ld
OBJCOPY	:= $(GCCPREFIX)objcopy
OBJDUMP	:= $(GCCPREFIX)objdump
NM	:= $(GCCPREFIX)nm

# Native commands
NCC	:= gcc $(CC_VER) -pipe
NATIVE_CFLAGS := $(CFLAGS) $(DEFS) $(LABDEFS) -I$(TOP) -MD -Wall
TAR	:= gtar
PERL	:= perl

# Compiler flags
# -fno-builtin is required to avoid refs to undefined functions in the kernel.
# Only optimize to -O1 to discourage inlining, which complicates backtraces.
CFLAGS := $(CFLAGS) $(DEFS) $(LABDEFS) -O1 -fno-builtin -I$(TOP) -MD

CFLAGS += -fno-omit-frame-pointer
CFLAGS += -std=gnu99
CFLAGS += -static
CFLAGS += -Wall -Wno-format -Wno-unused -Werror
ifeq ($(ARCH), riscv)
CFLAGS += -mabi=lp64 -march=rv64imafdc_zicsr_zifencei -mcmodel=medany
#CFLAGS += -fno-pic
else
CFLAGS += -m32 
CFLAGS += gstabs
endif
#CFLAGS += -fno-pic  #FIXME: 

# -fno-tree-ch prevented gcc from sometimes reordering read_ebp() before
# mon_backtrace()'s function prologue on gcc version: (Debian 4.7.2-5) 4.7.2
CFLAGS += -fno-tree-ch

#CFLAGS += -I$(TOP)/net/lwip/include \
	  -I$(TOP)/net/lwip/include/ipv4 \
	  -I$(TOP)/net/lwip/jos
CFLAGS += -I$(TOP)/

# Add -fno-stack-protector if the option exists.
CFLAGS += $(shell $(CC) -fno-stack-protector -E -x c /dev/null >/dev/null 2>&1 && echo -fno-stack-protector)

ifeq ($(ARCH),riscv)
# Common linker flags
LDFLAGS := -m elf64lriscv
else
LDFLAGS := -m elf_i386
endif

# Linker flags for JOS user programs
ULDFLAGS := -T user/$(ARCH)/user.ld

GCC_LIB := $(shell $(CC) $(CFLAGS) -print-libgcc-file-name)

# Lists that the */Makefrag makefile fragments will add to
OBJDIRS :=

# Make sure that 'all' is the first target
all:

# Eliminate default suffix rules
.SUFFIXES:

# Delete target files if there is an error (or make is interrupted)
.DELETE_ON_ERROR:

# make it so that no intermediate .o files are ever deleted
.PRECIOUS: %.o $(OBJDIR)/boot/%.o $(OBJDIR)/kern/%.o \
	   $(OBJDIR)/lib/%.o $(OBJDIR)/fs/%.o $(OBJDIR)/net/%.o \
	   $(OBJDIR)/user/%.o

ifeq ($(ARCH), riscv)
# current version of gcc doesn't support gstabs maybe dwarf
KERN_CFLAGS := $(CFLAGS) -DJOS_KERNEL -gdwarf
USER_CFLAGS := $(CFLAGS) -DJOS_USER -gdwarf
else
KERN_CFLAGS := $(CFLAGS) -DJOS_KERNEL -gstabs
USER_CFLAGS := $(CFLAGS) -DJOS_USER -gstabs
endif

# Update .vars.X if variable X has changed since the last make run.
#
# Rules that use variable X should depend on $(OBJDIR)/.vars.X.  If
# the variable's value has changed, this will update the vars file and
# force a rebuild of the rule that depends on it.
$(OBJDIR)/.vars.%: FORCE
	$(V)echo "$($*)" | cmp -s $@ || echo "$($*)" > $@
.PRECIOUS: $(OBJDIR)/.vars.%
.PHONY: FORCE


# Include Makefrags for subdirectories
include boot/arch/$(ARCH)/Makefrag
include kern/$(ARCH)/Makefrag
include lib/$(ARCH)/Makefrag
include user/$(ARCH)/Makefrag
include fs/Makefrag
#include net/Makefrag


CPUS ?= 1

ifeq ($(ARCH), riscv)

QEMUOPTS = -M virt -m 256M -serial mon:stdio
QEMUOPTS += $(shell if $(QEMU) -nographic -help | grep -q '^-D '; then echo '-D qemu.log'; fi)
QEMUOPTS += -smp $(CPUS)
QEMUOPTS += -bios $(TOP)/opensbi/fw_jump.bin
QEMUOPTS += -drive if=pflash,unit=0,format=raw,file=$(OBJDIR)/kern/kernel.img 

IMAGES = $(OBJDIR)/kern/kernel.img

else
PORT7	:= $(shell expr $(GDBPORT) + 1)
PORT80	:= $(shell expr $(GDBPORT) + 2)
QEMUOPTS = -drive file=$(OBJDIR)/kern/kernel.img,index=0,media=disk,format=raw -serial mon:stdio
QEMUOPTS += $(shell if $(QEMU) -nographic -help | grep -q '^-D '; then echo '-D qemu.log'; fi)
IMAGES = $(OBJDIR)/kern/kernel.img
QEMUOPTS += -smp $(CPUS)
QEMUOPTS += -drive file=$(OBJDIR)/fs/fs.img,index=1,media=disk,format=raw
IMAGES += $(OBJDIR)/fs/fs.img
QEMUOPTS += -net user -net nic,model=e1000 -redir tcp:$(PORT7)::7 \
	   -redir tcp:$(PORT80)::80 -redir udp:$(PORT7)::7 -net dump,file=qemu.pcap
QEMUOPTS += $(QEMUEXTRA)
endif

.gdbinit: .gdbinit.tmpl
	sed "s/localhost:1234/localhost:$(GDBPORT)/" < $^ > $@

QEMUDEBUG = -gdb tcp::$(GDBPORT) -S

gdb:
ifeq ($(ARCH), riscv)
	riscv64-unknown-elf-gdb -n -x .gdbinit-riscv
else
	gdb -n -x .gdbinit #FIXME: i386 use the native gdb when use prefix gdb it will cause error no color
endif

pre-qemu: .gdbinit
#	QEMU doesn't truncate the pcap file.  Work around this.
	@rm -f qemu.pcap

qemu: $(IMAGES) pre-qemu
	$(QEMU) $(QEMUOPTS)

qemu-nox: $(IMAGES) pre-qemu
	@echo "***"
	@echo "*** Use Ctrl-a x to exit qemu"
	@echo "***"
	$(QEMU) -nographic $(QEMUOPTS)

qemu-gdb: $(IMAGES) pre-qemu
	@echo "***"
	@echo "*** Now run 'make gdb'." 1>&2
	@echo "***"
	$(QEMU) $(QEMUOPTS) $(QEMUDEBUG)

qemu-nox-gdb: $(IMAGES) pre-qemu
	@echo "***"
	@echo "*** Now run 'make gdb'." 1>&2
	@echo "***"
	$(QEMU) -nographic $(QEMUOPTS) $(QEMUDEBUG)

print-qemu:
	@echo $(QEMU)

print-gdbport:
	@echo $(GDBPORT)

# For deleting the build
clean:
	rm -rf $(OBJDIR) .gdbinit jos.in qemu.log jos.out*

realclean: clean

distclean: realclean
	rm -rf conf/gcc.mk

ifneq ($(V),@)
GRADEFLAGS += -v
endif

grade:
	@echo $(MAKE) clean
	@$(MAKE) clean || \
	  (echo "'make clean' failed.  HINT: Do you have another running instance of JOS?" && exit 1)
	./grade-lab$(LAB) $(GRADEFLAGS)


prep-%:
	$(V)$(MAKE) "INIT_CFLAGS=${INIT_CFLAGS} -DTEST=`case $* in *_*) echo $*;; *) echo user_$*;; esac`" $(IMAGES)

run-%-nox-gdb: prep-% pre-qemu
	$(QEMU) -nographic $(QEMUOPTS) $(QEMUDEBUG)

run-%-gdb: prep-% pre-qemu
	$(QEMU) $(QEMUOPTS) $(QEMUDEBUG)

run-%-nox: prep-% pre-qemu
	$(QEMU) -nographic $(QEMUOPTS)

run-%: prep-% pre-qemu
	$(QEMU) $(QEMUOPTS)

# For network connections
which-ports:
	@echo "Local port $(PORT7) forwards to JOS port 7 (echo server)"
	@echo "Local port $(PORT80) forwards to JOS port 80 (web server)"

nc-80:
	nc localhost $(PORT80)

nc-7:
	nc localhost $(PORT7)

telnet-80:
	telnet localhost $(PORT80)

telnet-7:
	telnet localhost $(PORT7)

# This magic automatically generates makefile dependencies
# for header files included from C source files we compile,
# and keeps those dependencies up-to-date every time we recompile.
# See 'mergedep.pl' for more information.
$(OBJDIR)/.deps: $(foreach dir, $(OBJDIRS), $(wildcard $(OBJDIR)/$(dir)/*.d))
	@mkdir -p $(@D)
	@$(PERL) mergedep.pl $@ $^

-include $(OBJDIR)/.deps

always:
	@:

.PHONY: all always \
	handin git-handin tarball tarball-pref clean realclean distclean grade handin-prep handin-check
