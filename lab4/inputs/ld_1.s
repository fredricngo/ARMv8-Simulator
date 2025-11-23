mov x1, 0x1000
mov x4, 3
mov x5, 4
lsl X1, X1, 16
stur x4, [x1, 0x0]
ldur X6, [X1, 0x0]
hlt 0
