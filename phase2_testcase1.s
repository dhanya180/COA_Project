.data
sum: .word 0        
.text
    addi x1, x0, 1       # x1 = 1 (initial value)
    addi x2, x0, 6       # x2 = 6 (loop counter + 1 to handle bne logic)
    addi x3, x0, 0       # x3 = 0 (sum)

Loop:
    add x3, x3, x1       # sum += x1
    addi x1, x1, 1       # x1++
    addi x2, x2, -1      # x2--
    bne x0, x2, Loop     # If x2 != 0, loop again
    
    sw x3, 0(x0)         # Store the result in memory at address 0 (sum label)
    ecall                # End of program
