.text

movz X1, 0x1
b L1
add  X1, X1, #100        // should be skipped
L1:
add  X1, X1, #3          // X1 = 1 + 3 = 4
movz X2, 0x2
b L2
add  X2, X2, #10         // skipped
add  X2, X2, #20         // skipped
add  X2, X2, #30         // skipped
L2:
add  X2, X2, #5          // X2 = 2 + 5 = 7
movz X3, 0x3
b L3
L3:
add  X3, X3, #7          // X3 = 3 + 7 = 10
movz X4, 0x4
b A
add  X4, X4, #111        // skipped
A:
b B
add  X4, X4, #222        // skipped
B:
add  X4, X4, #6          // X4 = 4 + 6 = 10

add  X5, X5, #1000       // skipped

END:
add  X5, X5, #5          // X5 = 5 + 5 = 10

HLT 0
