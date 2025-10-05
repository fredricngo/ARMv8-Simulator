.text
cmp X11, X11
bge foo
add X2, X0, 10

bar:
HLT 0

foo:
cmp X11, X13
bge bar
add X3, X0, 10
HLT 0
