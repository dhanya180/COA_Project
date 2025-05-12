.data
    array:  .word 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16
    sum:    .word 0, 0, 0, 0   # Space for partial sums
    msg1:   .string "Core "
    msg2:   .string " partial sum: "
    msg3:   .string "Final sum: "

.text
main:
    # Determine the portion of the array this core will process
    li x6, 16    # Total array size
    li x7, 4     # Number of cores

    # Calculate chunk size
    div x8, x6, x7   # x8 = chunk size (4)
    mul x9, x1, x8   # x9 = start index for this core (CID * chunk size)

    # Prepare for partial sum computation
    li x10, 0        # x10 will be partial sum
    
    # Print debug message about core and start index
    li x5, 1
    beq x1, x0, skip_debug1
    
    la x11, msg1
    ecall
    mv x11, x1
    ecall
    la x11, msg2
    ecall

skip_debug1:
    # Compute partial sum
    mv x11, x9       # Current index
    add x12, x9, x8  # Ending index for this core

sum_loop:
    beq x11, x12, sum_done  # Exit loop when index reaches chunk end

    # Load array element
    la x13, array
    slli x14, x11, 2  # Multiply index by 4 (word size)
    add x13, x13, x14 # Calculate array address
    lw x15, 0(x13)    # Load array element

    # Add to partial sum
    add x10, x10, x15

    # Increment index
    addi x11, x11, 1
    j sum_loop

sum_done:
    # Print partial sum (for non-core 0)
    beq x1, x0, skip_debug2
    mv x11, x10
    ecall    # Print partial sum

skip_debug2:
    # Store partial sum to shared memory
    la x3, sum       # Base of sum array
    mul x4, x1, 4    # Offset based on core ID
    add x3, x3, x4   # Address for this core's partial sum
    sw x10, 0(x3)    # Store partial sum

    # Synchronization point
    sync

    # Only core 0 does final computation
    bne x1, x0, done

    # Compute final sum
    la x3, sum
    lw x5, 0(x3)     # First partial sum
    lw x6, 4(x3)     # Second partial sum
    add x5, x5, x6
    lw x6, 8(x3)     # Third partial sum
    add x5, x5, x6
    lw x6, 12(x3)    # Fourth partial sum
    add x5, x5, x6

    # Print final sum
    la x11, msg3
    ecall
    mv x11, x5
    ecall

done:
    ecall    # Exit program