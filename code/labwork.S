.global delay
.global time2string
.global enable_interrupt
  # labwork.S
  # Written 2015-2017 by F Lundevall
  # Skeleton file for IS1200/IS1500 lab 1.
  # The macros PUSH and POP are in the public domain.
  # Please add your own code at the end of the file.
  #
  # Please keep the two macros PUSH and POP unchanged
  #

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

# delay the program
# argument: $a0, number of milliseconds to delay
delay:
    # preserve $s0
    addi $sp, $sp, -4
    sw $s0, 0($sp)

    MilliSec:
        ble $a0, $zero, EndMilliSec
        nop

        addi $a0, $a0, -1
        move $s0, $zero
        Loop:
            li $t0, 3400 # delay constant
            ble $t0, $s0, EndLoop
            nop
            addi $s0, $s0, 1
            j Loop
            nop
        EndLoop:
            j MilliSec
            nop
    EndMilliSec:

    # restore $s0
    lw $s0, 0($sp)
    addi $sp, $sp, 4

    jr $ra
    nop


# time to string representation
# params:
# $a0 - a memory location
# $a1 - NBCD-coded time digits
# will write the minutes and seconds separated
# by a colon (:) to the memory address and also a null byte
# will write DING instead when it is on 00:00
time2string:
    # preserve $s0 and $s1.
    # s0 is used for saving memeory adress in $a0
    # s1 is used for saving return adress since hexasc is called
    addi $sp, $sp, -12

    sw $s0, 8($sp)
    sw $s1, 4($sp)
    sw $s2, 0($sp)

    # save arguments due to changes to $a0 and $ra
    # when calling hexasc
    move $s0, $a0
    move $s1, $ra

    # save $a1 since it cant be guaranteed after a function call
    move $s2, $a1

    # go to branch ding on new minutes (xx:00)
    move $t5, $s2
    andi $t5, $t5, 0x000000FF
    beq $t5, $zero, ding
    nop

    # shift all the minutes and seconds
    # to numbers that can be arguments for hexasc

    srl $a0, $s2, 12 # xm:ss
    #andi $a0, $a0, 0x000F
    jal hexasc
    nop
    sb $v0, 0($s0)

    srl $a0, $s2, 8 # mx:ss
    #andi $a0, $a0, 0x000F
    jal hexasc
    nop
    sb $v0, 1($s0)

    # colon
    li $t0, 0x3A
    sb $t0, 2($s0)

    srl $a0, $s2, 4 # mm:xs
    #andi $a0, $a0, 0x000F
    jal hexasc
    nop
    sb $v0, 3($s0)

    #andi $a0, $s2, 0x000F # mm:sx
    move $a0, $s2
    jal hexasc
    nop
    sb $v0, 4($s0)

    # store null byte
    li $t0, 0x00
    sb $t0, 5($s0)

    # get the correct return adress
    move $ra, $s1

    # restore $s0 and $s1, $s2
    lw $s2, 0($sp)
    lw $s1, 4($sp)
    lw $s0, 8($sp)
    addi $sp, $sp, 12

    jr $ra
    nop

    ding:
    	# write DING in ascii to memory address in $a1
    	li $t0,0x00000044  # D
    	sb $t0, 0($s0)

    	li $t0,0x00000049  # D
    	sb $t0, 1($s0)

    	li $t0,0x0000004E  # N
    	sb $t0, 2($s0)

    	li $t0,0x00000047  # G
    	sb $t0, 3($s0)

    	# store null byte
    	li $t0, 0x00
    	sb $t0, 4($s0)

    	# get the correct return adress
    	move $ra, $s1

    	# restore $s0 and $s1, $s2
	lw $s2, 0($sp)
    	lw $s1, 4($sp)
    	lw $s0, 8($sp)
    	addi $sp, $sp, 12

    	jr $ra
    	nop


# param: $a0, a number between 0-15
# return: $v0, ASCII code digit 0-9 or A-F.
# $v0 will only contain 7 bits.
hexasc:
    andi $a0, $a0, 0x0000000F # and away everything except 4 bits
    li $t0, 0xA
    ble $t0, $a0, L1
    nop

    # A 0-9 number to ASCII digit 0-9
    addi $v0, $a0, 0x30

    # only 7 bits should be returned, rest should be zeroes
    andi $v0, $v0, 0x0000007F

    j end
    nop

    L1:
        # A 10-15 number to ASCII letters A-F
        addi $v0, $a0, 55
        # only 7 bits should be returned, rest should be zeroes
        andi $v0, $v0, 0x0000007F
    end:
        jr $ra
        nop
