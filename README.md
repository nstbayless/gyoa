# gyoa
v0.3

Git Your Own Adventure: an engine for crowd-written choose-your-own-adventure stories.

Requires
 - [libgit2](https://github.com/libgit2/libgit2) version 0.23 or higher
 - boost system
 - boost filesystem


The [rapidxml](http://rapidxml.sourceforge.net/) library is used for storing data. Ships with repository.

## Build instructions

```
cd debug
make
```

## Setting up with Eclipse CDT

- use c++11 dialect
- include folders: `./include`, `./rapidxml`
- include libraries: `boost_system`, `boost_filesystem`, `git2`
