.text
stur X1, [X10, #0x60]
ldur X2, [X10, #0x60]
stur X11, [X10, #0x68]
ldur X12, [X10, #0x68]
stur X12, [X10, #0x70]

stur W11, [X10, #0x90]
stur W12, [X10, #0x98]

ldur W14, [X10, #0x60]
ldur W15, [X10, #0x68]

HLT 0
