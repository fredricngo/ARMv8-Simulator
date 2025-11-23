mov x0, 0x1000
lsl x0, x0, 4
add x1, x0, 0x2
add x2, x0, 0x4
add x3, x0, 0x6
add x4, x0, 0x8
add x5, x0, 0xa
add x6, x0, 0xc
add x7, x0, 0xe
add x8, x0, 0x10
add x9, x0, 0x12

lsl x1, x1, 12
lsl x2, x2, 12
lsl x3, x3, 12
lsl x4, x4, 12
lsl x5, x5, 12
lsl x6, x6, 12
lsl x7, x7, 12
lsl x8, x8, 12
lsl x9, x9, 12

stur x1, [x1, 0x0]
stur x2, [x2, 0x0]
stur x3, [x3, 0x0]
stur x4, [x4, 0x0]
stur x5, [x5, 0x0]
stur x6, [x6, 0x0]
stur x7, [x7, 0x0]
stur x8, [x8, 0x0]
stur x9, [x9, 0x0]

ldur x19, [x9, 0x0]
ldur x18, [x8, 0x0]
ldur x17, [x7, 0x0]
ldur x16, [x6, 0x0]
ldur x15, [x5, 0x0]
ldur x14, [x4, 0x0]
ldur x13, [x3, 0x0]
ldur x12, [x2, 0x0]
ldur x11, [x1, 0x0]
hlt 0
