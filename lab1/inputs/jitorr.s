.text

// Test 1: Basic OR operation
movz X0, 0x00FF    // X0 = 0x00FF (255)
movz X1, 0xFF00    // X1 = 0xFF00 (65280)
orr X2, X0, X1     // X2 = X0 | X1 = 0xFFFF (65535)

// Test 2: OR with zero (identity)
movz X3, 0x1234
movz X4, 0x0
orr X5, X3, X4     // X5 = 0x1234 | 0x0 = 0x1234

// Test 3: OR with all ones
movz X6, 0x5555
movz X7, 0xFFFF
orr X8, X6, X7     // X8 = 0x5555 | 0xFFFF = 0xFFFF


// Test 5: OR with zero register
movz X11, 0x1111
orr X12, X11, XZR // X12 = 0x1111 | 0 = 0x1111

// Test 6: OR writing to zero register (should remain 0)
movz X13, 0x2222
movz X14, 0x3333
orr XZR, X13, X14  // X31 should still be 0

// Test 7: Complement patterns
movz X15, 0x0F0F
movz X16, 0xF0F0
orr X17, X15, X16  // X17 = 0x0F0F | 0xF0F0 = 0xFFFF

// Test 8: Single bit patterns
movz X18, 0x0001   // Bit 0 set
movz X19, 0x8000   // Bit 15 set
orr X20, X18, X19  // X20 = 0x8001 (bits 0 and 15 set)

// Test 9: Chain of OR operations
movz X21, 0x1000
movz X22, 0x0100
movz X23, 0x0010
orr X24, X21, X22  // X24 = 0x1100
orr X25, X24, X23  // X25 = 0x1110

HLT 0
