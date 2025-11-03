.text
mov X1, 30
mov X2, 0
mov X6, 6
mov X7, 30

foo:
mov X3, 1
mov X3, 1
mov X3, 1
add X2, X2, 1
mov X3, 1
mov X3, 1
mov X3, 1
mov X3, 1
mul X6, X6, X7
cmp X1, X2
bgt foo


mov X1, 4
mov X2, 0


HLT 0

