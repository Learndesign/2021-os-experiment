#include <asm/sbidef.h>
#include <asm/sbiasm.h>

// #define BOOT_LOADER_SIG_OFFSET 0x1fe
// #define OS_SIZE_LOC (BOOT_LOADER_SIG_OFFSET - 2)
// #define OS_SIZE_LOC 0x1fc

.equ os_size_loc, 0x502001fc

// 1. kernel address (move kernel to here ~)
.equ kernel, 0x50200000 

// 2. kernel main address (jmp here to start kernel main!)
.equ kernel_main, 0x50200000 

// 3. copy bootload to new address
.equ bootloader_copy, 0x50300000     

// 4. restart address of bootloader 
.equ bootloader_start, 0x50300018        

.text
.global main

main:	
	//C-core:move the bootloader to 0x50500000
	la a0, bootloader_copy     
	li a1, 1               
	li a2, 0				
	SBI_CALL SBI_SD_READ
	call bootloader_start

	// fence on all memory and I/O
	fence
	fence.i

	// print a \n to avoid system output error...
	li a0, 10
	SBI_CALL SBI_CONSOLE_PUTCHAR

	// 1) task1 call BIOS print string "It's bootblock!"
	la       a0, msg             #*str
	SBI_CALL SBI_CONSOLE_PUTSTR

	//same to kernel0 or kernel1
getnum:
	SBI_CALL SBI_CONSOLE_GETCHAR
	li a1, -1
	beq a0,a1, getnum
	li a2, 48
	beq a0, a2, load_kernel0

load_kernel1:
	li a0, 10
	SBI_CALL SBI_CONSOLE_PUTCHAR

	la       a0, test1            #*str
	SBI_CALL SBI_CONSOLE_PUTSTR

	// 2) kernel0 in sector1 and kernek1 in sector2
	la a0, kernel
	la s1, os_size_loc
	lh a1, 0(s1) 
	li a2, 2
	SBI_CALL SBI_SD_READ
	la t0, kernel_main
	jr t0  

load_kernel0:
	la       a0, test0           #*str
	SBI_CALL SBI_CONSOLE_PUTSTR

	la a0, kernel
	la s1, os_size_loc
	lh a1, 0(s1) 
	li a2, 1
	SBI_CALL SBI_SD_READ
	la t0, kernel_main
	jr t0  

// while(1) --> stop here
stop:
	j stop
	nop

.data
msg: .string "It's a bootloader...\n\r"
test0: .string "test0 succeed\n\r"
test1: .string "test1 succeed\n\r"