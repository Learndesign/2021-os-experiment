SHELL := /bin/bash
HOST_CC = gcc -w
CROSS_PREFIX = riscv64-unknown-linux-gnu-
CC = ${CROSS_PREFIX}gcc -w
AR = ${CROSS_PREFIX}ar
OBJDUMP = ${CROSS_PREFIX}objdump
CFLAGS = -O0 -nostdlib -T riscv.lds -Wall -mcmodel=medany -Iinclude -Idrivers -nostdinc -g -fvar-tracking -ffreestanding
USER_CFLAGS = -O0 -nostdlib -T user_riscv.lds -Wall -mcmodel=medany -Itest -Itiny_libc/include -Iinclude -nostdinc -g -fvar-tracking
USER_LDFLAGS = -L./ -ltiny_libc
DISK = /dev/sdb

BOOTLOADER_ENTRYPOINT = 0x50200000
START_ENTRYPOINT = 0x50301000
KERNEL_ENTRYPOINT = 0xffffffc050400000

ARCH = riscv
ARCH_DIR = ./arch/$(ARCH)
CFLAGS += -Iarch/$(ARCH)/include
USER_CFLAGS += -Iarch/$(ARCH)/include
USER_LDFLAGS += $(ARCH_DIR)/crt0.o

SRC_BOOT 	= $(ARCH_DIR)/boot/bootblock.S
SRC_HEAD    = $(ARCH_DIR)/kernel/head.S $(ARCH_DIR)/kernel/boot.c payload.c ./libs/string.c
SRC_ARCH	= $(ARCH_DIR)/kernel/trap.S $(ARCH_DIR)/kernel/entry.S $(ARCH_DIR)/kernel/start.S $(ARCH_DIR)/sbi/common.c $(ARCH_DIR)/kernel/smp.S
SRC_DRIVER	= ./drivers/screen.c ./drivers/net.c ./drivers/plic.c ./drivers/emacps/xemacps.c ./drivers/emacps/xemacps_bdring.c ./drivers/emacps/xemacps_control.c ./drivers/emacps/xemacps_example_util.c ./drivers/emacps/xemacps_g.c ./drivers/emacps/xemacps_hw.c ./drivers/emacps/xemacps_intr.c ./drivers/emacps/xemacps_main.c
SRC_INIT 	= ./init/main.c
SRC_INT		= ./kernel/irq/irq.c
SRC_LOCK	= ./kernel/locking/lock.c #./kernel/locking/futex.c
SRC_SCHED	= ./kernel/sched/sched.c ./kernel/sched/time.c ./kernel/sched/smp.c
SRC_MM	    = ./kernel/mm/mm.c ./kernel/mm/ioremap.c
SRC_SYSCALL	= ./kernel/syscall/syscall.c
SRC_LIBS	= ./libs/string.c ./libs/printk.c

SRC_LIBC	= ./tiny_libc/printf.c ./tiny_libc/string.c ./tiny_libc/mthread.c ./tiny_libc/syscall.c ./tiny_libc/invoke_syscall.S \
				./tiny_libc/time.c ./tiny_libc/mailbox.c ./tiny_libc/rand.c ./tiny_libc/atol.c
SRC_LIBC_ASM	= $(filter %.S %.s,$(SRC_LIBC))
SRC_LIBC_C	= $(filter %.c,$(SRC_LIBC))
SRC_FS      = ./kernel/fs/fs.c ./kernel/fs/fs_tool.c 
SRC_USER	= ./test/test_shell.elf ./test/test_fs.elf 
			  
#SRC_TEST2	= ./test/test_project2/test_scheduler1.c ./test/test_project2/test_scheduler2.c ./test/test_project2/test_lock2.c ./test/test_project2/test_sleep.c ./test/test_project2/test_timer.c
#./test/test_project3/test_wait.c
#SRC_TEST3	= ./test/test_project3/test_kill.c ./test/test_project3/test_condition.c ./test/test_project3/test_semaphore.c ./test/test_project3/test_barrier.c ./test/test_project3/test_sanguo.c

SRC_MAIN	= ${SRC_ARCH} ${SRC_INIT} ${SRC_INT} ${SRC_DRIVER} ${SRC_LOCK} ${SRC_SCHED} ${SRC_MM} ${SRC_SYSCALL} ${SRC_LIBS} ${SRC_FS}

SRC_IMAGE	= ./tools/createimage.c
SRC_ELF2CHAR	= ./tools/elf2char.c
SRC_GENMAP	= ./tools/generateMapping.c

.PHONY:all main bootblock clean

all: elf2char createimage image asm # floppy

bootblock: $(SRC_BOOT) riscv.lds#little change
	${CC} ${CFLAGS} -o bootblock $(SRC_BOOT) -e main -Ttext=${BOOTLOADER_ENTRYPOINT} -Wall 

kernelimage: $(SRC_HEAD) riscv.lds#little change
	${CC} ${CFLAGS} -o kernelimage $(SRC_HEAD) -Ttext=${START_ENTRYPOINT} -Wall 

payload.c: elf2char main
	./elf2char main > payload.c

user: $(SRC_USER) elf2char generateMapping
	echo "" > user_programs.c
	echo "" > user_programs.h
	for prog in $(SRC_USER); do ./elf2char --header-only $$prog >> user_programs.h; done
	for prog in $(SRC_USER); do ./elf2char $$prog >> user_programs.c; done
	./generateMapping user_programs
	mv user_programs.h include/
	mv user_programs.c kernel/

libtiny_libc.a: $(SRC_LIBC_C) $(SRC_LIBC_ASM) user_riscv.lds
	for libobj in $(SRC_LIBC_C); do ${CC} ${USER_CFLAGS} -c $$libobj -o $${libobj/.c/.o}; done
	for libobj in $(SRC_LIBC_ASM); do ${CC} ${USER_CFLAGS} -c $$libobj -o $${libobj/.S/.o}; done
	${AR} rcs libtiny_libc.a $(patsubst %.c, %.o, $(patsubst %.S, %.o,$(SRC_LIBC)))

$(ARCH_DIR)/crt0.o: $(ARCH_DIR)/crt0.S
	${CC} ${USER_CFLAGS} -c $(ARCH_DIR)/crt0.S -o $(ARCH_DIR)/crt0.o

%.elf : %.c user_riscv.lds libtiny_libc.a $(ARCH_DIR)/crt0.o
	${CC} ${USER_CFLAGS} $< ${USER_LDFLAGS} -o $@

main: $(SRC_MAIN) user riscv.lds
	${CC} ${CFLAGS} -o main $(SRC_MAIN)  ./kernel/user_programs.c -Ttext=${KERNEL_ENTRYPOINT}

createimage: $(SRC_IMAGE)
	${HOST_CC} ${SRC_IMAGE} -o createimage -ggdb -Wall
elf2char: $(SRC_ELF2CHAR)
	${HOST_CC} ${SRC_ELF2CHAR} -o elf2char -ggdb -Wall
generateMapping: $(SRC_GENMAP)
	${HOST_CC} ${SRC_GENMAP} -o generateMapping -ggdb -Wall

image: bootblock kernelimage createimage
	./createimage --extended bootblock kernelimage
# dd if=/dev/zero of=image oflag=append conv=notrunc bs=512 count=65
# dd if=/dev/zero of=disk bs=512 count=1M
# dd if=image of=disk conv=notrunc
	dd if=/dev/zero of=image oflag=append conv=notrunc bs=512 count=2500
clean:
	rm -rf bootblock image kernelimage createimage main payload.c libtiny_libc.a
	rm include/user_programs.h kernel/user_programs.c
	find . -name "*.o" -exec rm {} \;

floppy:
	sudo fdisk -l ${DISK}
	sudo dd if=image of=${DISK}2 conv=notrunc

asm:
	${OBJDUMP} -d main > kernel.txt
	${OBJDUMP} -d kernelimage > kernelimage.txt
	
