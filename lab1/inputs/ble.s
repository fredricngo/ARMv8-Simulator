.text
cmp X11, X11
ble foo
add X2, X0, 10

bar:
HLT 0

foo:
cmp X11, X12
ble bar
add X3, X0, 10
HLT 0
