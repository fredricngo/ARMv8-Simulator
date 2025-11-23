.text
mov x1, 100
mov x2, 0
mov x10, 0x1000
mov x3, 0x45b
lsl x10, x10, 16

foo:
add x2, x2, 2
add x10, x10, 8
stur x3, [x10, 0x0]
stur x3, [x10, 0x10]

cmp x1, x2
bgt foo
ldur x5, [x10, 0x0]
ldur x6, [x10, 0x10]
hlt 0
