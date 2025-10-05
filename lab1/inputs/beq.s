.text
cmp X11, X11
beq foo
add X2, X0, 10

bar:
HLT 0

foo:
cmp X11, X11
beq bar
add X3, X0, 10
HLT 0
