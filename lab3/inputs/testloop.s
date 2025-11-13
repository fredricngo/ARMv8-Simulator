.text
mov X1, 3
mov X2, 0

foo:
mov X3, 1
mov X3, 1
mov X3, 1
add X2, X2, 1
cmp X1, X2
bgt foo


mov X1, 4
mov X2, 0


HLT 0
b foo
