# Service discovery in LF OS

Most classic OS services (file system access, graphics, ... everything exposed by drivers) are implemented in
userspace in LF OS, meaning those are normal programs and if you want to use those services in your programs,
you have to find them somehow.

The initial way of doing this is (and still used at least while booting the system), is using a service
registry in the kernel, where programs register themselves as implementing a given service and other programs
can ask the kernel via syscall to find an instance of a service.

This pattern should work just fine for anything there is only one instance of, but would give the work of
finding the correct instance of a given service to the requesting program ("which file system is my root file
system? which graphics server should I paint on?"), together with more complexity in the service
implementations - as they would need to give something to not only identify which service they implement but
also which instance they are, in a way that should be understandable by system administrators.

Besides that, having e.g. multiple (root) file system services was also meant as a way to containerize
applications, a program executing `chroot` just does a "use this file system as my root file system instead of
my current one"). If you know `fork` away from this `chroot`-ed process, how is the fork expected to know it's
root file system? OK, that could bubble through in a variable .. but how can we **prevent** the fork from just
accessing the previous root file system or any other file system?

To solve this, processes are going to be initialized with the usual `[executable, arguments, environment]`
plus a list of service instances to use `[root file system = $foo, graphics server = $bar]`. The
kernel-supported service discovery is stripped down to one instance per service type and intended only for
bootstrapping the system, e.g. everything that needs a `file system` waiting until a `file system` is
registered, one of the bootloader-loaded drivers (hopefully) registering itself as one at some point, which
then allows the other processes to fully initialize themselves.
