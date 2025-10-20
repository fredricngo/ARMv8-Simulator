.text
add X1, X2, 1
add X2, X0, 5
add X3, X2, X2
add X4, X3, X2
adds X5, X3, X4
adds X6, X5, 0xff
adds X7, X6, 1
adds X8, X7, 4
and X9, X8, X7
and X10, X9, X8
ands X11, X8, X9
ands X12, X10, X11
b foo
add X13, X0, 10
foo:
add X14, X9, 11
b bar
bar:
add X15, X2, X8


check_beq:
cmp X3, X4
beq bcnot
add X16, X16, 1
add X16, X16, 2

bcnot:
add X16, X16, 3
add X16, X16, 4

cmp X3, X3
beq bctake
add X17, X17, 1
add X17, X17, 2

bctake:
add X17, X17, 3
add X17, X17, 4


check_bne:
cmp X3, X4
bne bnnot
add X18, X18, 1
add X18, X18, 2

bnnot:
add X18, X18, 3
add X18, X18, 4

cmp X3, X3
bne bntake
add X19, X19, 1
add X19, X19, 2

bntake:
add X19, X19, 3
add X19, X19, 4

check_bgt:
cmp X3, X4
bgt bgtnot
add X20, X20, 1
add X20, X20, 2

bgtnot:
add X20, X20, 3
add X20, X20, 4

cmp X4, X3
bgt bgttake
add X21, X21, 1
add X21, X21, 2

bgttake:
add X21, X21, 3
add X21, X21, 4


check_blt:
cmp X4, X3
blt bltnot
add X22, X22, 1
add X22, X22, 2

bltnot:
add X22, X22, 3
add X22, X22, 4

cmp X3, X4
blt blttake
add X23, X23, 1
add X23, X23, 2

blttake:
add X23, X23, 3
add X23, X23, 4


check_bge:
cmp X3, X4
bge bgenot
add X24, X24, 1
add X24, X24, 2

bgenot:
add X24, X24, 3
add X24, X24, 4

cmp X3, X3
bge bgetake
add X25, X25, 1
add X25, X25, 2

bgetake:
add X25, X25, 3
add X25, X25, 4


check_ble:
cmp X4, X3
ble blenot
add X26, X26, 1
add X26, X26, 2

blenot:
add X26, X26, 3
add X26, X26, 4

cmp X3, X4
ble bletake
add X27, X27, 1
add X27, X27, 2

bletake:
add X27, X27, 3
add X27, X27, 4




HLT 0

