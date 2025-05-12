    .data 
sum:    .word 0, 0, 0, 0  

    .text
    addi  x2, x1, 1        
    la    x3, sum         
    mul   x4, x1, 4        
    add   x3, x3, x4       
    sw    x2, 0(x3)        

    sync    

    li    x5, 0            
    bne   x1, x5, Done     

sum_computation:
    la    x3, sum
    lw    x5, 0(x3)       
    lw    x2, 4(x3)       
    add   x5, x5, x2       
    lw    x2, 8(x3)        
    add   x5, x5, x2      
    lw    x2, 12(x3)       
    add   x5, x5, x2       

Done:
    nop
