    .data
arr: .word 3, 2, 0, 5, 1
n:   .word 5

    .text
    la   x10, n
    lw   x1, 0(x10)       
    addi x2, x1, -1       

    la   x3, arr          
    addi x4, x0, 0        

Outer_Loop:
    beq  x4, x2, Done    
    addi x5, x0, 0        

Inner_Loop:
    sub  x6, x2, x4      
    beq  x5, x6, End_Inner
    add  x8, x5, x5      
    add  x8, x8, x5      
    add  x8, x8, x5      
    add  x7, x3, x8      
    lw   x9, 0(x7)        
    lw   x10, 4(x7)      

    ble  x9, x10, Skip_Swap
    sw   x10, 0(x7)
    sw   x9, 4(x7)

Skip_Swap:
    addi x5, x5, 1        
    bne  x0, x0, Inner_Loop

End_Inner:
    addi x4, x4, 1       
    bne  x0, x0, Outer_Loop

Done:
    nop                 