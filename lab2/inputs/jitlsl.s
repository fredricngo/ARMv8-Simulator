.text

// Test 2: Shift by larger amounts
movz X5, 1
mov X1, 0
mov X2, #100
mov X3, #100
lsl X6, X5, 8          // X6 = 1 << 8 = 256
lsl X7, X5, 16         // X7 = 1 << 16 = 65536
lsl X8, X5, 32         // X8 = 1 << 32 = 0x100000000

HLT 0 
