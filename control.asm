    clr $7             # setup temp zero reg

    addi $0, $7, 0x4
    sll $0, $0, 4        # v0 = 0x0040

    addi $1, $7, 0x1
    sll $1, $1, 8
    addi $1, $1, 0x1
    sll $1, $1, 4        # v1 = 0x1010

    addi $2, $7, 0xF    # v2 = 0x000F

    addi $3, $7, 0xF
    sll $3, $3, 4        # v3 = 0x00F0

    addi $4, $7, 0x0    # t0 = 0x0000

    addi $5, $7, 0x1
    sll $5, $5, 4         # a0 = 0x0010

    addi $6, $7, 0x5    # a1 = 0x0005

    addi $7, $7, 0xF
    sll $7, $7, 4
    addi $7, $7, 0xF
    sll $7, $7, 8        # $7 = 0xFF00
    sw $7, 10($4)

    clr $7
    addi $7, $7, 0xF
    sll $7, $7, 4
    addi $7, $7, 0xF    # $7 = 0x00FF
    sw $7, 12($4)        # store 0x00FF

    clr $7
    addi $7, $7, 0x1
    sll $7, $7, 8        # reference val for if = x0100
    sw $7, 14($4)        # store 0x0100
    j loop

exit:
    j end

loop:
    beqz $6, exit
    dec $6, $6
    lw $4, 0($5)
    lw $7, 14($7)        # load 0x0100 from mem

    ble $4, $7, else    # if($4 <= $7) branch
    sra $0, $0, 3        # $0 = $0 / 8
    or $1, $1, $0        # $1 = $1 | $0
    clr $7
    lw $7, 10($7)        # load 0xFF00 from mem
    sw $7, 0($5)        # store 0xFF00
    j cont

else:
    sll $2, $2, 2        # $2 = $2 * 4
    xor $3, $3, $2        # $3 = $3 ^ $2
    clr $7
    lw $7, 12($7)        # load 0x00FF from mem
    sw $7, 0($5)        # store 0x00FF

cont:
    clr $7
    addi $5, $5, 2
    j loop

end: