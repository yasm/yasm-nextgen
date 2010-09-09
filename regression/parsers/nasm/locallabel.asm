foo:
db .bar
..@bar2:
db .baz
.bar:
db .baz
.baz:
bar:
.bar:
db .bar
db foo.bar
db bar.bar
db ..@bar2

