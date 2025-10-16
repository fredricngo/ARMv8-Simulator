.text

// Simple STURB test without large addresses
movz X10, #100        // Small offset from wherever X10 starts
movz W0, #0x42
sturb W0, [X10, #0]   // Store relative to X10's initial value
ldurb W1, [X10, #0]   // Load it back

HLT 0 
