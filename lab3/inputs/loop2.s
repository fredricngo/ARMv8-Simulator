.text
mov X1, 1000
mov X2, 0
mov X6, 2
mov X7, 40

foo:
mov X3, 1
mov X3, 1
mov X3, 1
add X2, X2, 1
mov X3, 1
mov X3, 1
mov X3, 1
mov X3, 1
add X6, X6, X7
cmp X1, X2
bgt foo


mov X1, 4
mov X2, 0


HLT 0

