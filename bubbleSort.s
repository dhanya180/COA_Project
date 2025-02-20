.data
arr: .word 0x3 0x2 0x0 0x5 0x1 
str: .string "\nAfter Bubble Sort : "
space: .string " "

.text
addi x1 x0 5 #n
addi x2 x1 -1 #n-1
addi x4 x0 0 #i

Loop1:
    beq x4 x1 exit
    addi x5 x0 0 #j
    sub x6 x2 x4 #n-1-i
    la x3 arr
    ble x5 x6 Loop2
Loop2:
    lw x7 0(x3)
    lw x8 4(x3)
    ble x7 x8 exit1
    beq x5 x6 exit2
    sw x8 0(x3)
    sw x7 4(x3)
     
exit1:
    addi x3 x3 4
    addi x5 x5 1
    ble x5 x6 Loop2
exit2:
    addi x4 x4 1
    ble x4 x2 Loop1       
exit:
    li a7 4
    la a0 str
    ecall
    la x3 arr
print:
    beq x11 x1 exit3
    li a7 1
    lw a0 0(x3)
    ecall
    li a7 4
    la a0 space
    ecall
    addi x3 x3 4
    addi x11 x11 1
    j print
exit3:
    li a7 10
    ecall