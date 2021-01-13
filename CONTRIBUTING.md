<img align="right" height="100" src="LF OS.svg">

# Contributing to LF OS

Woah, you wanna contribute? You don't know how happy that will make me o.o

Here are some guidelines, infos where to find help and stuff.


## Canonical sources

The main project location is at https://praios.lf-net.org/littlefox/lf-os_amd64. There is also an issue tracker
and CI system, building all the branches on changes and also building the docs. Sources are mirrored to
https://github.com/LittleFox94/lf-os_amd64.


## Docs

Canocical documentation source for the main development branch is at https://littlefox.narf.press/lf-os_amd64.
The HTMLs are generated automagically by Doxygen from the contents of the `doc` directory and comments in the code.

Speaking about comments in the code: please use documentation comments on your new functions in the header files.
I know I don't do this very often myself, but I try to.

Docs are probably missing a lot and might be outdated. This is a single-person project right now after all x.x
I'm always available to chat, via IRC (Freenode LittleFox), Twitter (@0x0a\_fox) and whatever other contact
channel you may find. Don't feel bad for asking! If you have to ask, there are docs missing - so it's my fault, not
yours.


## Language

Main human language is english, used for the docs, commit changelogs and stuff. Important: be a good person, don't
make me have to write a code of conduct. I will if neccessary (if you need there to be one before contributing,
tell me, it's neccessary then (even if you end up not contributing after all - that's ok)).

Also try to be nice in comments, changelogs and alike, tho I can totally relate to just fsck everything .. let's
keep it to a minimum, ok? :)


## Code style

The code style in LF OS is a slight but not total mess. In general, code is preferred to look something like this:

```c
void foo(char* bar, const char* baz) {
    int a   = 42;
    int xyz = 23;

    if(bar && ! baz) {
        return;
    }
    else {
        kprintf(baz, bar);
    }
}
```

## Testing

There is some unit test infrastructure for generic components in the kernel, we need more infra. Please run the
`test` target before submitting code, also please test if LF OS is still starting - with `run` **and** `run-kvm`
targets - the latter one catching more errors (alignment issues, uninitialized memory, ..).


## Sending your changes to me

Github pull requests are accepted, as are diffs sent by email to me (look in `git log` for my address) or you
creating an account on my Gitlab instance and create a merge request there. You can also send me a link to your
repo, asking me to merge it - a real merge request ^^


## Perks of contributing (and a danger)

You will forever be in the `git log` of the project (**beware**: this could also be a bad thing for you).

After something you made is accepted into the main development branch, you can send me a physical address to send
you some [stickers](https://twitter.com/0x0a_fox/status/1264228960348577792) ([in natural habitat](https://twitter.com/0x0a_fox/status/1275065796834680833/photo/1)) - as long as I have some ^^'
