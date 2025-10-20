.text
mov X3, 0x1
mov X4, 0x2
b foo
add X13, X0, 10
foo:
add X14, X9, 11
b bar
bar:
add X15, X2, X8



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


HLT 0
