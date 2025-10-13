.text 

movz X1, 5
adds X2, X1, #10 // X2 = 15, flags: N=0, Z=0
adds x3, X1, #-5 // X3 = 0, flags: N=0, Z=1

HLT 0
