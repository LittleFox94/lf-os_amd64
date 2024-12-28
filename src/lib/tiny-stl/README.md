# tiny-stl - a tiny, platform-independent C++ STL implementation

Let's make the C++ ecosystem a tiny bit better, with a STL implementation
without any hard dependencies on any operating system the program might run on.
Basically taking the nice containers, algorithms, functional helpers and co
from the STL for use everywhere.

This means some part of the C++ spec will just not be possible to be
implemented by this library, while other parts might only be partly possible.
At some point, we could add a clear abstraction between the system and the
system-independent stuff, so that porting this to new platforms is easy and
straight-forward, but for now the focus is on actual platform-independence.

Platform-independence means no libc or other platform libraries required. You
can use this in your freestanding (non-hosted) project.

As the very first step, only things needed for the LF OS kernel are implemented
\- but everything implemented is supposed to match the C++ standards. Nothing
known at the time of implementation as deprecated will be added, removing
deprecated things might happen at any point. We are tracking the latest C++
standard here.

## Why?

That's what I want to ask the C++ standard people: why is the STL made of
several different functionalities, why intermingle containers, algorithms and
co. with platform-abstractions like filesystems, threads, ..? Shouldn't this be
two libraries closely working together, but each usable independently from the
other?
