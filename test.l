main:
    xor $7, $7, $7
    addi $7, $7, 0xFF

    xor $0, $0, $0
    addi $0, $0, 5

    addi $7, $7, -2
    sw $0, ($7)

    # test is 0xFD

    xor $0, $0, $0
    addi $0, $0, 0xFD

    lw $1, 0($0)
    addi $1, $1, 1

    addi $7, $7, -2
    sw $1, ($7)




