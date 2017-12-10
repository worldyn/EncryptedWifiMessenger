.macro  PUSH reg
  addi  $sp,$sp,-4
  sw  \reg,0($sp)
.endm

.macro  POP reg
  lw  \reg,0($sp)
  addi  $sp,$sp,4
.endm

.global mul_low
.global mul_upper

  .text

# $a0: factor a
# $a1: factor b
# returns the lower word of product a*b
mul_low:
  # Perform unsigned multiplication.
  multu $a0,$a1
  # Put LO in $v0 to return.
  mflo $v0
  jr $ra
  nop


# $a0: factor a
# $a1: factor b
# returns the upper word of product a*b
mul_upper:
  # Perform unsigned multiplication.
  multu $a0,$a1
  # Put HI in $v0 to return.
  mfhi $v0
  jr $ra
  nop

