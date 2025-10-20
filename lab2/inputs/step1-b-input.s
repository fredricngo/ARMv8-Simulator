.text
mov X1, 0x1000
mov X2, 1
mov X3, 4
mov x4, 5
lsl X1, X1, 16
mov X4, 0x1234
lsr X2, X2, 1
mov X3, 1
mov X8, 0x10
stur X4, [X1, 0x0]
sturb W4, [X1, 0x6]
ldur X6, [X1, 0x0]
ldur X7, [X1, 0x4]
ldurb W8, [X1, 0x6]
HLT 0

