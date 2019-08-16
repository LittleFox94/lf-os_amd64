# gdbserver-profiler - gsp

`gsp` is a profiling toolchain connecting to a gdbserver via UNIX socket or
TCP/IPv6. After reaching the start symbol (default: entry of provided image),
the tracer `gsp-trace` single steps through the program, until the stop symbol
is reached (default: never stops automatically).

The resulting trace file contains addresses for every executed instruction and
can be converted to a symbolized trace file in the following format with
`gsp-syms`.

```
_start
_start(+0x2)
main
main(+0x1)
main(+0x2)
main(+0x6)
...

```

Now, to have a visualized version of this, you can use `gsp-stackcollapse` to
convert the symbolized trace file to a stack trace file compatible with
[FlameGraph](https://github.com/brendangregg/FlameGraph).

