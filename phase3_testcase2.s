# All Cores:
    LI t0, 0             # Initialize sum[CID] to 0
    LI t1, 0             # Loop index
    LI t2, 2             # Number of elements per core
    MUL t3, CID, t2      # Compute the starting index for each core

Loop:
    ADD t4, t3, t1       # Compute the address for a[i]
    LW t5, 0(t4)         # Load a[i]
    ADD t0, t0, t5       # Partial sum update
    ADDI t1, t1, 1       # Increment index
    BNE t1, t2, Loop     # Loop until 2 elements are processed

# Synchronize all Cores
    SYNC                 # Wait for all cores to complete partial sum

# Core 1 only:
    BEQ CID, 1, Aggregate

    J Done

Aggregate:
    LI t6, 2             # Start from Core 2
Aggregate_Loop:
    LW t7, 0(t6)         # Load partial sum from other core
    ADD t0, t0, t7       # Aggregate sum
    ADDI t6, t6, 1       # Next core
    BNE t6, 5, Aggregate_Loop # Loop until Core 4

    # Print the final sum (use appropriate print call for your simulator)
    PRINT t0

Done:
    NOP                  # End of execution
