# Test program for RISC-V simulator
.data
data_label: .word 5, 10, 15

# Basic arithmetic instructions
.text
ADD X1, X2, X3        # X1 = X2 + X3 (1 + 2 = 3)
ADDI X4, X5, 10       # X4 = X5 + 10 (0 + 10 = 10)
SUB X6, X1, X4        # X6 = X1 - X4 (3 - 10 = -7)

# Immediate loading
LI X7, 15             # X7 = 15
LA X8, data_label   # X8 = address of data_label

# Branch instructions
BEQ X1, X0, equal_label  # Branch if X1 == X0 (3 != 0, so no branch)
BNE X1, X0, not_equal_label # Branch if X1 != X0 (3 != 0, so branch)
BLE X6, X7, less_equal_label #Branch if X6 <= X7 (-7 <=15, so branch)
J always_jump

equal_label:
    LI X9, 1          # This should not be executed
    J end

not_equal_label:
    LI X9, 2          # This should be executed: X9 = 2

less_equal_label:
     LI X10, 4 #X10 =4
always_jump:
    LI X10, 3         # This should be executed: X10 = 3 (overwriting the last li)

# Memory access instructions
LW X11, 0(X8)         # X11 = value at address in X8 (data_label)
SW X7, 4(X8)          # Store X7 at address (X8 + 4)

end:
# ECALL
ECALL



