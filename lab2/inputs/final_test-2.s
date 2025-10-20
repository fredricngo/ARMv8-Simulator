.text


add X2, X4, 17
cbnz X2, bar
add X2, X0, 1

foo:
add X8, X9, 13
b bar

bar:
add X10, X2, X8

add X2, X4, 17
cbnz X3, bar2
add X8, X9, 19
cbz X3, bar2
add X2, X0, 11

foo2:
add X8, X9, 1
b bar2

bar2:
add X10, X2, X8


lsl X0, X11, 3
lsr X0, X11, 2

cbz X12, target
add X1, X2, X3
target:


cbnz X12, target2
add X1, X2, X3
target2:


mov X9, #3
mov X10, #8
sub X11, X0, #2

add X9, X8, 10
add X10, X8, 7
add X11, X8, 23
eor X12, X10, X11
eor X13, X10, X9

mov X1, 0x1000
lsl X1, X1, 16
mov X10, 10
stur X10, [X1, 0x0]
mov X12, 2
stur X12, [X1, 0x10]
ldur X13, [X1, 0x0]
ldur X14, [X1, 0x10]

mov X1, 0x1000
lsl X1, X1, 16
mov X10, 0x1234
stur X10, [X1, 0x0]
sturb W10, [X1, 0x10]
ldur X13, [X1, 0x0]
ldurb W14, [X1, 0x10]


mov X1, 0x1000
lsl X1, X1, 16
mov X10, 0x1234
stur X10, [X1, 0x0]
sturh W10, [X1, 0x10]
ldur X13, [X1, 0x0]
ldurh W14, [X1, 0x10]

add X9, X8, 10
add X10, X8, 7
lsl X12, X10, 1
lsl X13, X9, 2
lsl X14, X9, 0

add X9, X8, 0x100
lsr X13, X9, 2
lsr X14, X9, 0
lsr X15, X9, 1
lsr X16, X9, 2
lsr X17, X9, 33
lsr X18, X9, 63

mov X10, #8
mov X11, #9
mul X12, X10, X11
mul X13, X10, X10

add X9, X8, 10
add X10, X8, 7
add X11, X8, 23
orr X12, X10, X11
orr X13, X10, X9

mov X1, 0x1000
lsl X1, X1, 16
mov x28, 0x1000
lsl x28, x28, 16
mov x0, #0x2174
mov X10, 10
stur X10, [X1, 0x0]
stur X0, [x28, 0xc]
stur X12, [X1, 0x10]
ldur X13, [X1, 0x0]
ldur X14, [X1, 0x10]

mov X1, 0x1000
lsl X1, X1, 16
mov X10, 0x1234
stur X10, [X1, 0x0]
sturb W10, [X1, 0x10]
ldur X13, [X1, 0x0]
ldur X14, [X1, 0x10]
ldurb W15, [X1, 0x10]

mov X1, 0x1000
lsl X1, X1, 16
mov X10, 0x1234
stur X10, [X1, 0x0]
sturh W10, [X1, 0x10]
ldur X13, [X1, 0x0]
ldurh W14, [X1, 0x10]

mov X1, 0x1000
lsl X1, X1, 16
mov x0, 0x2174
lsl x0, x0, 32
add x0, x0, 0x126
sturh w0, [x1, 0x0]
sturh w0, [X1, 0x10]
ldur X13, [X1, 0x0]
ldur X14, [X1, 0x10]
HLT 0

