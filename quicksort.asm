clr $0
clr $1

lw $1, ($0)  # store n
addi $0, $0, 2


dec $6, $1  # 5 and 6 are low and high
clr $5

clr $2
addi $2, $2, 0xFF # stack pointer

jal quicksort
j end

quicksort:

#store high, low and ra on the stack
    addi $2, $2, -6
    sw $5, ($2) #low
    sw $6, 2($2) #high

    ble $6, $5, jump2return
    j skipReturn
jump2return:
    j return
skipReturn:
    jal partition # $4 is the return
    sw $4, 4($2) # store the return on the stack
    lw $5, ($2) # load the low
    dec $6, $4 # decrement pi

    jal quicksort
    lw $4, 4($2)
    inc $5, $4
    lw $6, 2($2)
    jal quicksort
return:
    addi $2, $2, 6
    r

partition:
    addi $2, $2, -4
    sw $5, ($2) #low
    sw $6, 2($2) #high

    # load the pivot
    sll $5, $5, 1
    add $5, $0, $5
    lw $4, ($5) # P

    lw $5, ($2) # i
    lw $7, 2($2) #j
partition_loop:
    ble $7, $5, jump2partition_return
    j skip_partition_return
jump2partition_return:
    j partition_return
skip_partition_return:
    # get arr[i]
    while1:
    #arr[i] is less than pivot
    sll $3, $5, 1
    add $3, $0, $3
    lw $3, ($3)
    ble $3, $4, secondCondition
    j firstLoopEnd
secondCondition:
    # i is less than or equal to high - 1
    addi $1, $6, 0
    dec $1, $1 # high - 1
    ble $5, $1, loop
    j firstLoopEnd
loop:
    inc $5, $5

    j while1

firstLoopEnd:

while2:
    # load arr[j]
    sll $1, $7, 1
    add $1, $0, $1
    lw $1, ($1)

while2_first_condition:
    ble $1, $4, jump2secondLoopEnd
    j skipSecondLoopEnd
jump2secondLoopEnd:
    j secondLoopEnd
skipSecondLoopEnd:
    j while2_second_condition
while2_second_condition:
    lw $3, ($2)
    inc $3, $3
    ble $3, $7, j_decrement_loop
    j secondLoopEnd
j_decrement_loop:
    dec $7, $7
    j while2
secondLoopEnd:
    ble $7, $5, jump2partition_loop
    j skip_partition_loop
jump2partition_loop:
    j partition_loop
skip_partition_loop:
    addi $2, $2, -4
    sw $5 ($2)
    sw $7, 2($2)

    #swap arr[$5] and arr[$7] using $1 and $3
    sll $1, $5, 1
    add $1, $0, $1
    lw $5, ($1)

    sll $3, $7, 1
    add $3, $0, $3
    lw $7, ($3)

    sw $7, ($1)
    sw $5 ($3)

    lw $5 ($2)
    lw $7, 2($2)
    addi $2, $2, 4

    j partition_loop

partition_return:
    # swap arr[low] and arr[j]
    lw $5, ($2)
    sll $5, $5, 1
    add $5, $0, $5
    lw $6, ($5)

    sll $4, $7, 1
    add $4, $0, $4
    lw $3, ($4)

    sw $6, ($4)
    sw $3, ($5)

    clr $4
    addi $4, $7, 0
    addi $2, $2, 4
    r
end:
j end