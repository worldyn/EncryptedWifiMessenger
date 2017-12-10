.global enable_interrupt

.macro	PUSH reg
	addi	$sp,$sp,-4
    sw	\reg,0($sp)
.endm

.macro	POP reg
    lw	\reg,0($sp)
	addi	$sp,$sp,4
.endm
  #
  # Please add your own code below this line
  #

enable_interrupt:
  ei 
  jr $ra
  nop



