    .data
arr: .word 42,98,77,22,64,70,18,87,55,35,92,91,53,36,31,19,14,21,67,28
      .word 34,94,2,72,78,51,59,47,50,46,9,5,84,24,57,90,79,65,97,60
      .word 30,41,89,48,40,95,17,38,16,4,29,49,45,6,44,100,83,71,85,32
      .word 15,73,86,63,13,3,52,68,61,69,88,7,39,75,80,76,81,99,23,8
      .word 82,96,10,43,20,27,66,12,11,56,33,54,1,74,62,93,25,58,37,26
str1: .string "\nThe sum of all the hundred elements is: "

    .text
    la   x18, arr        # x18 = base of arr
    addi x31, x0, 25     # x31 = 25
    bne  CID, x0, AfterSum0
    addi x15, x18, 0     # x15 = ptr
    addi x16, x0, 0      # x16 = sum
    addi x8,  x0, 0      # x8  = i
Loop0:
    beq  x8,  x31, AfterSum0
    lw   x25, 0(x15)
    add  x16, x16, x25
    sw   x16, 1020(x0)
    addi x15, x15, 4
    addi x8,  x8,  1
    bne  x8,  x31, Loop0
AfterSum0:
    bne  CID, x1, AfterSum1   
    addi x15, x18, 100
    addi x16, x0,  0
    addi x8,  x0,  0
Loop1:
    beq  x8,  x31, AfterSum1
    lw   x25, 0(x15)
    add  x16, x16, x25
    sw   x16, 1024(x0)
    addi x15, x15, 4
    addi x8,  x8,  1
    bne  x8,  x31, Loop1
AfterSum1:
    bne  CID, x2, AfterSum2
    addi x15, x18, 200
    addi x16, x0,  0
    addi x8,  x0,  0
Loop2:
    beq  x8,  x31, AfterSum2
    lw   x25, 0(x15)
    add  x16, x16, x25
    sw   x16, 1028(x0)
    addi x15, x15, 4
    addi x8,  x8,  1
    bne  x8,  x31, Loop2
AfterSum2:
    bne  CID, x3, AfterSum3
    addi x15, x18, 300
    addi x16, x0,  0
    addi x8,  x0,  0
Loop3:
    beq  x8,  x31, AfterSum3
    lw   x25, 0(x15)
    add  x16, x16, x25
    sw   x16, 1032(x0)
    addi x15, x15, 4
    addi x8,  x8,  1
    bne  x8,  x31, Loop3
AfterSum3:
    lw   x22, 1020(x0)
    lw   x23, 1024(x0)
    lw   x24, 1028(x0)
    lw   x25, 1032(x0)
    add  x23, x22, x23
    add  x24, x24, x23
    add  x25, x25, x24
    sw   x25, 1020(x0)

Done:
    nop     
