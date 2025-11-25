mov x0, 0x1
mov x1, 0x1
mov x0, 0x1
mov x1, 0x1
mov x0, 0x1
mov x1, 0x1

cmp x0, x1

add x2, x0, x1
add x2, x2, x1
add x3, x0, x1  // x3 = 1 + 1 = 2
add x3, x3, x1  // x3 = 2 + 1 = 3

hlt 0
