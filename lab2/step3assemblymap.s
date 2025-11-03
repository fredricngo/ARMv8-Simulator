.text
mov X3, 0x1                     // 0x400000
mov X4, 0x2                     // 0x400004
b foo                           // 0x400008
add X13, X0, 10                 // 0x40000C
foo:
add X14, X9, 11                 // 0x400010
b bar                           // 0x400014
bar:
add X15, X2, X8                 // 0x400018



cmp X3, X4                      // 0x40001C
beq bcnot                       // 0x400020
add X16, X16, 1                 // 0x400024
add X16, X16, 2                 // 0x400028

bcnot:
add X16, X16, 3                 // 0x40002C
add X16, X16, 4                 // 0x400030

cmp X3, X3                      // 0x400034
beq bctake                      // 0x400038
add X17, X17, 1                 // 0x40003C
add X17, X17, 2                 // 0x400040

bctake:
add X17, X17, 3                 // 0x400044
add X17, X17, 4                 // 0x400048



cmp X3, X4                      // 0x40004C
bne bnnot                       // 0x400050
add X18, X18, 1                 // 0x400054
add X18, X18, 2                 // 0x400058

bnnot:
add X18, X18, 3                 // 0x40005C
add X18, X18, 4                 // 0x400060

cmp X3, X3                      // 0x400064
bne