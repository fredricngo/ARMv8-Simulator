.text
cbnz X12, bar
add X2, X0, 10

foo:
HLT 0

bar:
cbnz X11, foo
add X3, X0, 10
HLT 0
