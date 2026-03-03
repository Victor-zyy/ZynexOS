# ZynexOS 

##  1. For RISC-V Running on x86-64 Platform

default ARCH = riscv

* compile for riscv 

```shell
make ARCH=riscv 
```

* run via qemu on x86-64 platform

```shell
make ARCH=riscv  qemu / qemu-nox
```

If you wanna exit qemu on terminal, use the keyboard binding `Ctrl+A and then X`

* debug using gdb and qemu

By the gdb debug protocol, a gdb server and a gdb client.
```shell
make ARCH=riscv qemu-gdb / qemu-nox-gdb
```
and then open another terminal typing
```shell
make ARCH=riscv gdb
```

* Additional Information for test

```shell
make ARCH=riscv print-qemu 
make ARCH=riscv print-gdbport
```

* clean target and garbage

```shell
make clean
make disclean
```

* For network connections

1. echo test 
One process run the command down below, another process run `make nc-7`
```shell
make run-echosrv-nox 
```
2. httpd test
One process run the command down below, another process run `make nc-80` or using firefox browser type `http://host:port/index.html` the host is the name of the computer and the port is the number directed to 80 on the GNUMakefile.
```shell
make run-httpd-nox
http://ubuntu:26002/index.html
```