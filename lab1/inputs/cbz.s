.text
cbz XZR, bar
add X2, X0, 10

foo:
HLT 0

bar:
cbz XZR, foo
add X3, X0, 10
HLT 0
