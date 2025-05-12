.data
num1:   .word 10        # First number
num2:   .word 20        # Second number
result: .word 0         # Space for the result

.text
    la x5, num1         # x5 = address of num1
    lw x6, 0(x5)        # x6 = value at num1

    la x5, num2         # x5 = address of num2
    lw x7, 0(x5)        # x7 = value at num2

    add x8, x6, x7      # x8 = x6 + x7

    la x5, result       # x5 = address of result
    sw x8, 0(x5)        # store x8 to result
