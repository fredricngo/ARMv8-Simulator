.text

movz X5, 1000
mov X1, 0
mov X2, #100
mov X3, #100
add X6, X5, 4095 //X6 should be 5095

add X7, X1, 0 //should be 0

movz X8, 1
mov X1, #100
mov X2, #100
mov X3, #100
add X9, X8, 1   // X9 = 2
mov X1, 0
mov X2, #100
mov X3, #100
add X10, X9, 2  // X10 = 4
mov X1, #100
mov X2, #100
mov X3, #100
add X11, X10, 4 // X11 = 8

HLT 0
