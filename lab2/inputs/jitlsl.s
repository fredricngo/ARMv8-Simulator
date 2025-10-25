.text

// Test 2: Shift by larger amounts
movz X5, 1
lsl X6, X5, 8          // X6 = 1 << 8 = 256
lsl X7, X5, 16         // X7 = 1 << 16 = 65536
lsl X8, X5, 32         // X8 = 1 << 32 = 0x100000000

// Test 3: Shift non-power-of-2 values
movz X9, 5
lsl X10, X9, 1         // X10 = 5 << 1 = 10
lsl X11, X9, 2         // X11 = 5 << 2 = 20
lsl X12, X9, 4         // X12 = 5 << 4 = 80

// Test 4: Shift with larger base values
movz X13, 0xFF         // X13 = 255
lsl X14, X13, 8        // X14 = 255 << 8 = 65280 (0xFF00)
lsl X15, X13, 16       // X15 = 255 << 16 = 0xFF0000

// Test 5: Maximum shift amount (63 bits)
movz X16, 1
lsl X17, X16, 63       // X17 = 1 << 63 = 0x8000000000000000

// Test 6: Shift that causes overflow/truncation
movz X18, 0xFFFF       // X18 = 65535
lsl X19, X18, 32       // X19 = 65535 << 32 = 0xFFFF00000000
lsl X20, X18, 48       // X20 = 65535 << 48 = 0xFFFF000000000000

// Test 7: Zero shifted
movz X21, 0
lsl X22, X21, 10       // X22 = 0 << 10 = 0

// Test 8: Shift from zero register
lsl X23, XZR, 5        // X23 = 0 << 5 = 0

// Test 9: Shift to zero register (result discarded)
movz X24, 42
lsl XZR, X24, 3        // XZR should remain 0

// Test 10: Pattern recognition - powers of 2
movz X25, 3            // Binary: 11
lsl X26, X25, 1        // X26 = 6    (Binary: 110)
lsl X27, X25, 2        // X27 = 12   (Binary: 1100)
lsl X28, X25, 3        // X28 = 24   (Binary: 11000)

HLT 0 
