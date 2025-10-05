.text
cmp X11, X12
bne foo
add X2, X0, 10

bar:
HLT 0

foo:
cmp X11, X12
bne bar
add X3, X0, 10
HLT 0
