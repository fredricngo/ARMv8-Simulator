main:
mov     x0, #0x2174          // 4 digits in decimal 
mov     x28, 0x1000
lsl     x28, x28, 16
mov     x2, 0
mov     x3, 10
b addDigits

foo:
hlt     0

addDigits:
add     x28, x28, #0x10
stur    x0, [x28,#12]
ldur    x1, [x28,#12]
add     x2, x2, 1
stur    x2, [x28,#16]
ldur    x3, [x28,#16]
sub     x28, x28, #0x10
b      foo



