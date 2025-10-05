.text
b foo
add X2, X0, 10

bar:
HLT 0

foo:
b bar
add X3, X0, 10
HLT 0
