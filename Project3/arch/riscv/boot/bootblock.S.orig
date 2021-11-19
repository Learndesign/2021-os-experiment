// use your bootblock.S here
#include <asm/sbidef.h>
#include <asm/sbiasm.h>

// #define BOOT_LOADER_SIG_OFFSET 0x1fe
// #define OS_SIZE_LOC (BOOT_LOADER_SIG_OFFSET - 2)
// #define OS_SIZE_LOC 0x1fc

.equ os_size_loc, 0x502001fc

// 1. kernel address (move kernel to here ~)
.equ kernel, 0x50201000 

// 2. kernel main address (jmp here to start kernel main!)
.equ kernel_main, 0x50201000    

// 3. bootloader new address (relocate)
.equ bootloader, 0x50500000     

// 4. real start of bootloader 
.equ bootloader_start, 0x5050001a        

.text
.global main

main:
	// fence on all memory and I/O
	fence
	fence.i

	// print a \n to avoid system output error...
	li a0, 10
	SBI_CALL SBI_CONSOLE_PUTCHAR

	// 1) task1 call BIOS print string "It's bootblock!"
	la       a0, msg             #*str
	SBI_CALL SBI_CONSOLE_PUTSTR

	// 2) task2 call BIOS read kernel in SD card and jump to kernel start
	la       a0, kernel          #mem_addr
	la       a3, os_size_loc
	lh       a1, (a3)            #num_of_blocks
	li       a2, 1               #block_id
	SBI_CALL SBI_SD_READ
	la       t0, kernel_main
	jr		 t0     

// while(1) --> stop here
stop:
	j stop
	nop

.data

msg: .string "It's a bootloader...\n\r"