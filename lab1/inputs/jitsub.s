.text
// Test 1: Basic subtraction operations
movz X0, #100
sub X1, X0, #25         // X1 = 100 - 25 = 75
sub X2, X0, #50         // X2 = 100 - 50 = 50
sub X3, X0, #100        // X3 = 100 - 100 = 0

// Test 2: Subtracting zero (identity)
movz X4, #42
sub X5, X4, #0          // X5 = 42 - 0 = 42


// Test 4: Creating negative results
movz X7, #5
sub X8, X7, #10         // X8 = 5 - 10 = -5
sub X9, X7, #20         // X9 = 5 - 20 = -15

// Test 5: Large immediate values (12-bit max = 4095)
movz X10, #5000
sub X11, X10, #4095     // X11 = 5000 - 4095 = 905
sub X12, X10, #1        // X12 = 5000 - 1 = 4999

// Test 6: Writing to zero register (result discarded)
movz X13, #200
sub X21, X13, #50       // XZR should remain 0

// Test 7: Chain of subtractions
movz X14, #1000
sub X15, X14, #100      // X15 = 900
sub X16, X15, #200      // X16 = 700
sub X17, X16, #300      // X17 = 400


// Test 9: Different register patterns
movz X21, #777
movz X22, #888
sub X23, X21, #100      // X23 = 777 - 100 = 677
sub X24, X22, #200      // X24 = 888 - 200 = 688

// Test 10: Edge case - subtract maximum immediate from small number
movz X25, #10
sub X26, X25, #4095     // X26 = 10 - 4095 = -4085 (negative)

// Test 11: Powers of 2 subtraction
movz X27, #64
sub X28, X27, #32       // X28 = 64 - 32 = 32
sub X29, X28, #16       // X29 = 32 - 16 = 16
sub X30, X29, #8        // X30 = 16 - 8 = 8

HLT 0
