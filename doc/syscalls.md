# LF OS amd64 syscalls

"This is going to be a long, boring document" -- Mara Sophie Grosch, 2019-09-09

"lolnope" -- Mara Sophie Grosch, some days later

"obsolete docs found /o\" -- Mara Sophie Grosch, 2019-10-09


## Using syscalls

When compiling the toolchain, the userspace side of the syscalls is generated automatically.

```c
#include <sys/syscalls.h>
```

See build/syscalls.h for a list of syscalls and their arguments.


## Implementation of syscalls

Syscalls in LF OS are defined in `src/syscalls.yml`. This YAML file is the data file for the code generator
`src/syscall-generator.pl` (which needs `YAML` installed via i.e. CPAN) which generates matching userspace
and kernel space code.

The actual syscall implementation is in functions which are already given the parsed arguments. Those are
actually mostly platform agnostic.

Never implement your client side syscalls without using the generator and `syscall.yml`. Calling conventions,
call numbers, registers and stuff might change without further notice until LF OS 1.0 is released (lol).

