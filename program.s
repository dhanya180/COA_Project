LABELx:
add x0 x0 x1
sub x0 x0 x1
bne x0 x2 LABELy
sw x0 0(x3)
LABELy:
sub x0 x0 x2
jal x0 LABELx