.text
movz    x1, 0x40        // x1 = 0x0000000000000040
lsl     x1, x1, #16     // x1 = 0x0000000000400000
add     x1, x1, #0x1C   // x1 = 0x000000000040001C (address of target_addr)
mov     x5, #0
mov     x6, #0
mov     x7, #0
mov     x8, #0
br      x1 
mov     x0, #100
target_addr:
mov     x0, #200
HLT     0
