# dedalo.cpp

[![Casual Maintenance Intended](https://casuallymaintained.tech/badge.svg)](https://casuallymaintained.tech/)

![logo](logo.png)

A strongly opinionated, Clang-first build system for C++.
Build your C++ projects with just more C++.

---

Inspired by [Swift](https://www.swift.org/documentation/server/guides/building.html) and [Zig](https://ziglang.org/learn/build-system/).
All you need to get a C++ project up and running is a C++ compiler, no other languages, config formats, or 3rd party libraries needed.

> NOTE: Atm only Clang (default) and GCC are supported


## You're in control
A Makefile is provided for convenience (ironic, I know), but it's trivial to compile it yourself.
The whole project is a single cpp file, so it's easy to modify, and distribute it alongside your project's source.
The "canon" name of the executable is `ddl`, but you can change that to whatever you want by changing `EXECUTABLE_NAME` in the Makefile
That way you could give it a more "official" name like `cpp` so you can do things like `cpp init` or `cpp build`.


## Commands
- `init`
- `build [TargetName]`
- `run [TargetName]`
- `test [TestFrameworkArgs]`
- `clean`
- `--version` or `-v`


## Getting started
Create a new project like this:
```bash
$ mdkir my_project
$ cd my_project
$ ddl init
$ ddl build
```

This will generate the following tree:
```bash
.
├── build
│   ├── bin
│   │   └── my_project
│   ├── build_script.so
│   ├── dep
│   │   └── main.cpp.d
│   ├─── json
│   │    └── main.cpp.json
│   └── obj
│       └── main.cpp.o
├── build.cpp
├── compile_commands.json
├── lib
└── src
│   └── main.cpp
└── tests
```
> NOTE: If the header of the test framework [Icaro](https://github.com/javiersalcedopuyo/Icaro) is present in the directory at the time of calling `init`, it'll also generate a `tests/test.cpp` file.

Running `build/bin/my_project` will print the usual "Hello, World!".

By default `ddl build` and `ddl run` will compile in Debug mode.
`Release`, 'Test', and `Debug` targets are provided by default with the following configuration:
```c++
{
    .name               = "Debug",
    .optimization_level = 0,
    .sanitizers         = ASan | UBSan,
    .defines            = { "DEBUG" },
    .compiler_args      = {
        "g",
        "Wall",
        "Werror",
        "pedantic" }
},
{
    .name               = "Test",
    .optimization_level = 0,
    .sanitizers         = ASan | UBSan,
    .defines            = { "TESTING" },
    .compiler_flags     = {
        "g",
        "Wall",
        "Wextra",
        "Werror",
        "pedantic" },
    .source_path        = "tests"
},
{
    .name               = "Release",
    .optimization_level = 3,
    .sanitizers         = No_Sanitizers,
    .defines            = { "RELEASE" },
    .compiler_args      = {
        "Wall",
        "Werror",
        "pedantic" }
}
```

All source files added to `src/` will be compiled, but you can exclude paths adding them to the target's `ignored_paths`.
Only `src/` and `lib/` are added to the list of include paths. You'll have to add the whole path for any nested files you want to include.

You can add libraries/dependencies to the project like this:
```c++
// build.cpp

// Dynamic linking with a system copy by default
project->add_dependency({ name = "lib_name" }):

// Dynamic linking with a system copy explicitly
project->add_dependency({
    .name     = "lib_name"
    .linking  = Linking::Dynamic,
    .location = Location::System }) ;

// Dynamic linking with a local copy
// (On macOS, it relies on the install name being the same as the file name)
project->add_dependency({
    .name     = "lib_name",
    .linking  = Linking::Dynamic,
    .location = Location::Local });

// Static linking: location is ignored and assumed `Local`
project->add_dependency({
    .name = "lib_name",
    .linking = Linking::Static });

// Single Header:
//  Just a dummy at the moment, but it'll verify that the `lib/lib_name` directory exists and it's not empty.
project->add_dependency({
    .name    = "lib_name",
    .linking = Linking::SingleHeader });
```
- `Local` libraries will look for binaries (static or dynamic) and headers in `lib/lib_name`
- `System` (aka installed) libraries don't need a directory in `lib`.
- Single Header libraries don't *need* to be in `lib` but it's recommended to place them there.


## Planned features
- [x] Single project config file, written in C++.
- [x] Auto-generated file structure.
- [x] Generation of `compile_commands.json`
- [x] Incremental builds
- [x] Multithreaded builds (Unbalanced)
- [x] `test` command
- [ ] Support for MSVC
- [ ] Local dependency management
- [ ] Remote depenency management and/or integration with Conan.
- [ ] Nested projects
- [ ] Native support for test frameworks like [Catch2](https://github.com/catchorg/Catch2) or [GoogleTest](https://github.com/google/googletest).
- [ ] Optional support for Make, Ninja, CMake and other build systems (for dependencies or as an alternative backend).
- [ ] Allow to choose the linker
- [ ] Support for C
- [ ] Support for Obj-C

