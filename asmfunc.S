@-----------------------------------------------------------@
@ LPC23xx 1startup code and asm functions (C)ChaN, 2008
@ FIQ/IRQ Handler added by Iori.y.ja, Jan,2012
@-----------------------------------------------------------@


.equ UND_Stack_Size,	0
.equ SVC_Stack_Size,	256	@ Used by ISRs and SWIs
.equ ABT_Stack_Size,	0
.equ FIQ_Stack_Size,	256	@ Used by FIQ handler
.equ IRQ_Stack_Size,	256	@ Requres ISR nesting level * 28 bytes
.equ USR_Stack_Size,	2048	@ Used by user mode programs
.equ Stack_Size,	(UND_Stack_Size + SVC_Stack_Size + ABT_Stack_Size + \
			FIQ_Stack_Size + IRQ_Stack_Size + USR_Stack_Size)
.equ Heap_Size,		0

.equ B_Irq,	0x80
.equ B_Fiq,	0x40
.equ B_Thumb,	0x20
.equ M_USR,	0x10
.equ M_FIQ,	0x11
.equ M_IRQ,	0x12
.equ M_SVC,	0x13
.equ M_ABT,	0x17
.equ M_UND,	0x1B
.equ M_SYS,	0x1F



@-----------------------------------------------------------@
@ Stack area (located in RAM)
@-----------------------------------------------------------@

.arm
.section .STACK, "w"
.align 3
Stack_Mem:
.space Stack_Size
.equ Stack_Top, Stack_Mem + Stack_Size



@-----------------------------------------------------------@
@ Heap area (located in RAM)
@-----------------------------------------------------------@

.section .HEAP, "w"
.align 3
HeapMem:
.if (Heap_Size > 0)
.space Heap_Size
.endif



@-----------------------------------------------------------@
@ Exception entries (located in ROM, address 0x00000000)
@-----------------------------------------------------------@

.section .VECTOR, "ax"
.arm

	LDR	PC, [PC, #24]	@ Reset entry, jump to reset handler
	LDR	PC, [PC, #24]	@ Undef entry, trap
	LDR	PC, [PC, #24]	@ SWI entry, jump to SWI handler
	LDR	PC, [PC, #24]	@ PAbt entry, trap
	LDR	PC, [PC, #24]	@ DAbt entry, trap
	.word	0		@ Reserved
	LDR	PC, [PC, #20]	@ IRQ entry, jump to IRQ handler
	LDR     PC, [PC, #20]	@ FIQ entry, trap

	.word	Reset_Handler	@ Reset handler
	.word	Trap		@ Undefined Instruction handler
	.word	SWI_Handler	@ Software Interrupt handler
	.word	Trap		@ Prefetch Abort handler
	.word	Trap		@ Data Abort handler
	.word	IRQ_Handler	@ IRQ handler
	.word	FIQ_Handler	@ FIQ handler

Trap:	B	Trap		@ Unused exception trap (infinite loop)



@-----------------------------------------------------------@
@ Reset Handler
@-----------------------------------------------------------@

.section .text, "ax"
.arm
Reset_Handler:

@.extern TargetResetInit
@	LDR	SP, =Stack_Top    @ Temporary stack for TargetResetInit()
@	LDR	R0, =TargetResetInit
@	MOV	LR, PC
@	BX	R0

@ Setup Stack for each mode
	LDR	R0, =Stack_Top

@ Enter Undefined Instruction Mode and set its Stack Pointer
	MSR	CPSR_c, #M_UND | B_Irq | B_Fiq
	MOV	SP, R0
	SUB	R0, R0, #UND_Stack_Size

@ Enter Abort Mode and set its Stack Pointer
	MSR	CPSR_c, #M_ABT | B_Irq | B_Fiq
	MOV	SP, R0
	SUB	R0, R0, #ABT_Stack_Size

@ Enter FIQ Mode and set its Stack Pointer
	MSR	CPSR_c, #M_FIQ | B_Irq | B_Fiq
	MOV	SP, R0
	SUB	R0, R0, #FIQ_Stack_Size

@ Enter IRQ Mode and set its Stack Pointer
	MSR	CPSR_c, #M_IRQ | B_Irq | B_Fiq
	MOV	SP, R0
	SUB	R0, R0, #IRQ_Stack_Size

@ Enter Supervisor Mode and set its Stack Pointer
	MSR	CPSR_c, #M_SVC | B_Irq | B_Fiq
	MOV	SP, R0
	SUB	R0, R0, #SVC_Stack_Size

@ Enter User Mode and set its Stack Pointer
	MSR	CPSR_c, #M_USR | B_Irq | B_Fiq
	MOV	SP, R0
	SUB	SL, SP, #USR_Stack_Size

@ Relocate .data section (Initialize with ROM data)
	LDR	R1, =_etext
	LDR	R2, =_data
	LDR	R3, =_edata
	CMP	R2, R3
	BEQ	DataIsEmpty
LoopRel:CMP	R2, R3
	LDRLO	R0, [R1], #4
	STRLO	R0, [R2], #4
	BLO	LoopRel
DataIsEmpty:

@ Clear .bss section (Initialize with 0)
	MOV	R0, #0
	LDR	R1, =__bss_start__
	LDR	R2, =__bss_end__
	CMP	R1,R2
	BEQ	BSSIsEmpty
LoopZI:	CMP	R1, R2
	STRLO	R0, [R1], #4
	BLO	LoopZI
BSSIsEmpty:

@ Start main()
.extern main
	LDR	R0, =main
	MOV	LR, PC
	BX	R0
MTrap:	B	MTrap	@ Trap if main() terminated



@-----------------------------------------------------------@
@ IRQ Handler
@ Prologue and Epilog for all ISRs are handled here
@-----------------------------------------------------------@

.equ LPC_BASE_VIC,	0xFFFFF000
.equ VIC_IntSelect,	0x00C
.equ VIC_IntEnable,	0x010
.equ VIC_IntEnClear,	0x014
.equ VIC_Protection,	0x020
.equ VIC_SWPriorityMask,0x024
.equ VIC_VectAddr0,	0x100
.equ VIC_VectPriority0,	0x200
.equ VIC_Address,	0xF00
.equ ticksrc,		0x10

.section .text, "ax"
.arm
IRQ_Handler:
	MSR	CPSR_c, #M_IRQ | B_Irq | B_Fiq
	SUB	LR, LR, #4		@ Adjust LR_irq and push it
	STMFD	SP!, {LR}
	LDR	LR, =LPC_BASE_VIC
	LDR	LR, [LR, #0]
	CMP	LR, #ticksrc		@test if src is task tick or not
	BEQ	TaskMgr

	MRS	LR, SPSR		@ Save SPSR need to be saved for nested interrupt
	STMFD	SP!, {R0-R3,IP,LR}	@ Push scratch/used registers and SPSR
	LDR	R0, =LPC_BASE_VIC	@ Get the ISR address pointed by VIC_VectAddr
	LDR	R0, [R0, #VIC_Address]

	MSR	CPSR_c, #M_IRQ		@ Enter IRQ mode and enable Irq
	MOV	LR, PC
	BX	R0

	MSR	CPSR_c, #M_IRQ | B_Irq	@ Enter IRQ mode and disable Irq
	LDMFD	SP!, {R0-R3,IP,LR}	@ Restore scratch/used registers and SPSR
	MSR	SPSR, LR		@ Restore SPSR_irq

	LDR	LR, =LPC_BASE_VIC	@ Issue EOI command to the VIC
	STR	LR, [LR, #VIC_Address]

	LDMFD	SP!, {PC}^		@ Reruen from the IRQ handler

.extern currentpr
TaskMgr:
	LDMFD	SP!, {LR}
	STMFD	SP!, {R0,R3}
	LDR	R3, =currentpr		@get currentpr addr
	LDR	R0, [R3, #0]		@currentpr points current process cell
savestatus:				@this is pointer for currentprocess cell
	STR	LR, [R0, #8]		@write previous PC on current process
	MOV	LR, R0
	LDR	R0, [R0, #0]
	STR	R0, [R3, #0]		@renew currentpr
	LDMFD	SP!, {R0,R3}
	STR	R1, [LR, #16]
	STR	R2, [LR, #20]
	STR	R0, [LR, #12]
	STR	R3, [LR, #24]
	STR	R4, [LR, #28]
	STR	R5, [LR, #32]
	STR	R6, [LR, #36]
	STR	R7, [LR, #40]
	STR	R8, [LR, #44]
	STR	R9, [LR, #48]
	STR	R10,[LR, #52]
	STR	R11,[LR, #56]
	STR	R12,[LR, #60]
	MSR	CPSR_c, #M_SYS | B_Irq | B_Fiq
	MOV	R12, SP
	MOV	R11, LR
	MSR	CPSR_c, #M_IRQ | B_Irq | B_Fiq
	STR	R12,[LR, #64]
	STR	R11,[LR, #68]
processchoice:
	LDR	R0, [LR, #0]		@(next process adr)
	MRS	R1, SPSR		@ Save SPSR need to be saved for nested interrupt
	STR	R1, [LR, #72]
	STMFD	SP!, {R0}	@ Push scratch/used registers and SPSR
	ldr	lr, .L27
	ldr	r2, .L27+4
	ldr	r0, [lr, #-4043]
	ldr	r1, [r2, #0]
	eor	ip, r0, #262144
	ldr	r1, [r1, #4]
	ldr	r0, .L27+8
	str	ip, [lr, #-4043]
	bl	printf
	ldr	r3, .L27+12
	mov	r2, #1
	str	r2, [r3, #0]
.LC3:
	.ascii	"p:%d\000"
	.space	3
.L27:
	.word	1073729535
	.word	currentpr
	.word	.LC3
	.word	-536854528
	.align	2
	LDMFD	SP!, {R0}	@ Restore scratch/used registers and SPSR
	MSR	CPSR_c, #M_IRQ | B_Irq | B_Fiq

	LDR	LR, [R0, #8]		@next process's PC
	STMFD	SP!, {LR}
getbackstatus:
	LDR	R1, [R0, #72]
	MSR	SPSR, R1
	LDR	R1, [R0, #16]
	LDR	R2, [R0, #20]
	LDR	R3, [R0, #24]
	LDR	R4, [R0, #28]
	LDR	R5, [R0, #32]
	LDR	R6, [R0, #36]
	LDR	R7, [R0, #40]
	LDR	R8, [R0, #44]
	LDR	R9, [R0, #48]
	LDR	R10,[R0, #52]
	LDR	R11,[R0, #56]
	LDR	R12,[R0, #60]
	MSR	CPSR_c, #M_SYS
	LDR	R13,[R0, #64]
	LDR	R0, [R0, #12]
	MSR	CPSR_c, #M_IRQ
	LDR	LR, =LPC_BASE_VIC	@ Issue EOI command to the VIC
	STR	LR, [LR, #VIC_Address]
	LDMFD	SP!, {PC}^


@-----------------------------------------------------------@
@ FIQ Handler
@ Prologue and Epilog for all ISRs are handled here
@-----------------------------------------------------------@

.section .text, "ax"
.arm
FIQ_Handler:
	SUB	LR, LR, #4		@ Adjust LR_fiq and push it
	STMFD	SP!, {LR}

	MRS	LR, SPSR		@ Save SPSR need to be saved for nested interrupt
	STMFD	SP!, {R0-R3,IP,LR}	@ Push scratch/used registers and SPSR
	LDR	R0, =LPC_BASE_VIC	@ Get the ISR address pointed by VIC_VectAddr
	LDR	R0, [R0, #VIC_Address]

	MOV	LR, PC
	BX	R0

	MSR	CPSR_c, #M_FIQ | B_Fiq	@ Enter IRQ mode and disable Irq
	LDMFD	SP!, {R0-R3,IP,LR}	@ Restore scratch/used registers and SPSR
	MSR	SPSR_cxsf, LR		@ Restore SPSR_irq

	LDR	LR, =LPC_BASE_VIC	@ Issue EOI command to the VIC
	STR	LR, [LR, #VIC_Address]

	LDMFD	SP!, {PC}^		@ Reruen from the IRQ handler



@-----------------------------------------------------------@
@ SWI Service (declared in interrupt.h, taskmgr.h)
@-----------------------------------------------------------@

.equ SWI_IRQ_DIS,	0
.equ SWI_IRQ_EN,	1
.equ SWI_FIQ_DIS,	2
.equ SWI_FIQ_EN,	3
.equ SWI_CLR_VECT,	4
.equ SWI_REG_VECT,	5

.equ NUM_SWIs,		6


.section .text, "ax"
.arm
SWI_Handler:
	LDR	IP, [LR, #-4]		@ Get swi instruction code (assuming ARM state)
	AND	IP, #0xFF		@ Get swi comment field (lower 8 bit)
	CMP	IP, #NUM_SWIs		@ Check range
	LDRLO	PC, [PC, IP, LSL #2]	@ Jump each service function when code is valid
	MOVS	PC, LR			@ Otherwise return
	.word	IRQDisable
	.word	IRQEnable
	.word	FIQDisable
	.word	FIQEnable
	.word	ClearVect
	.word	RegVect

IRQDisable:
	MRS	R0, SPSR
	ORR	R0, R0, #B_Irq
	MSR	SPSR_c, R0
	MOVS	PC, LR

IRQEnable:
	MRS	R0, SPSR
	BIC	R0, R0, #B_Irq
	MSR	SPSR_c, R0
	MOVS	PC, LR

FIQDisable:
	MRS	R0, SPSR
	ORR	R0, R0, #B_Fiq
	MSR	SPSR_c, R0
	MOVS	PC, LR

FIQEnable:
	MRS	R0, SPSR
	BIC	R0, R0, #B_Fiq
	MSR	SPSR_c, R0
	MOVS	PC, LR

ClearVect:
	LDR	IP, =LPC_BASE_VIC
	MVN	R0, #0				@ Disable all interrupts
	STR	R0, [IP, #VIC_IntEnClear]
	MOV	R0, R0, LSR #16			@ Unmask all interrupt levels
	STR	R0, [IP, #VIC_SWPriorityMask]
	MOV	R0, #1				@ Enable protection
	STR	R0, [IP, #VIC_Protection]
	STR	R0, [IP, #VIC_Address]		@ Issule EOI command
	MOVS	PC, LR

RegVect:
@interrupt source:R0, Corresponding handler:R1, Priority:R2, Irq or Fiq:R3
	CMP	R0, #32				@ Range check
	MOVCSS	PC, LR
	LDR	IP, =(LPC_BASE_VIC+VIC_VectAddr0)
	STR	R1, [IP, R0, LSL #2]		@ Set VICVectVectAddr<n>
	LDR	IP, =(LPC_BASE_VIC+VIC_VectPriority0)
	STR	R2, [IP, R0, LSL #2]		@ Set VICVectPriority<n>
	MOV	R1, #1
	MOV	R1, R1, LSL R0
	LDR	IP, =LPC_BASE_VIC
	LDR	R2, [IP, #VIC_IntSelect]	@ Set corresponding bit in the VICIntSelect
	BIC	R2, R1
	CMP	R3, #1
	ORREQ	R2, R1
	STR	R2, [IP, #VIC_IntSelect]
	STR	R1, [IP, #VIC_IntEnable]	@ Enable corresponding interrupt
	MOVS	PC, LR

.global IrqDisable
IrqDisable:
	SWI	SWI_IRQ_DIS
	BX	LR

.global IrqEnable
IrqEnable:
	SWI	SWI_IRQ_EN
	BX	LR

.global FiqDisable
FiqDisable:
	SWI	SWI_FIQ_DIS
	BX	LR

.global FiqEnable
FiqEnable:
	SWI	SWI_FIQ_EN
	BX	LR

.global ClearVector
ClearVector:
	SWI	SWI_CLR_VECT
	BX	LR

.global RegisterVector
RegisterVector:
	SWI	SWI_REG_VECT
	BX	LR



@-----------------------------------------------------------@
@ Fast Block Copy (declared in mci.c)
@-----------------------------------------------------------@

.global Load_Block
.arm
Load_Block:
	STMFD	SP!, {R4-R8}
	ANDS	IP, R1, #3
	BEQ	lb_align

	BIC	R1, #3
	MOV	IP, IP, LSL #3
	RSB	R8, IP, #32
	LDMIA	R1!, {R7}
lb_l1:	MOV	R3, R7
	LDMIA	R1!, {R4-R7}
	MOV	R3, R3, LSR IP
	ORR	R3, R3, R4, LSL R8
	MOV	R4, R4, LSR IP
	ORR	R4, R4, R5, LSL R8
	MOV	R5, R5, LSR IP
	ORR	R5, R5, R6, LSL R8
	MOV	R6, R6, LSR IP
	ORR	R6, R6, R7, LSL R8
	SUBS	R2, R2, #16
	STMIA	R0!, {R3-R6}
	BNE	lb_l1
	LDMFD	SP!, {R4-R8}
	BX	LR

lb_align:
	LDMIA	R1!, {R3-R6}
	SUBS	R2, R2, #16
	STMIA	R0!, {R3-R6}
	BNE	lb_align
	LDMFD	SP!, {R4-R8}
	BX	LR


.global Store_Block
.arm
Store_Block:
	STMFD	SP!, {R4-R8}
	ANDS	IP, R0, #3
	BEQ	sb_align

	MOV	IP, IP, LSL #3
	RSB	R8, IP, #32

	LDMIA	R1!, {R4-R7}
sb_p1:	STRB	R4, [R0], #1
	MOV	R4, R4, LSR #8
	TST	R0, #3
	BNE	sb_p1
	ORR	R4, R4, R5, LSL IP
	MOV	R5, R5, LSR R8
	ORR	R5, R5, R6, LSL IP
	MOV	R6, R6, LSR R8
	ORR	R6, R6, R7, LSL IP
	SUBS	R2, R2, #16
	STMIA	R0!, {R4-R6}

sb_l1:	MOV	R3, R7
	LDMIA	R1!, {R4-R7}
	MOV	R3, R3, LSR R8
	ORR	R3, R3, R4, LSL IP
	MOV	R4, R4, LSR R8
	ORR	R4, R4, R5, LSL IP
	MOV	R5, R5, LSR R8
	ORR	R5, R5, R6, LSL IP
	MOV	R6, R6, LSR R8
	ORR	R6, R6, R7, LSL IP
	SUBS	R2, R2, #16
	STMIA	R0!, {R3-R6}
	BNE	sb_l1

	MOV	R7, R7, LSR R8
sb_p2:	SUBS	IP, IP, #8
	STRB	R7, [R0], #1
	MOV	R7, R7, LSR #8
	BNE	sb_p2

	LDMFD	SP!, {R4-R8}
	BX	LR

sb_align:
	LDMIA	R1!, {R3-R6}
	SUBS	R2, #16
	STMIA	R0!, {R3-R6}
	BNE	sb_align
	LDMFD	SP!, {R4-R8}
	BX	LR

.end

