# Coding style

Write C-style C++. The project uses C++ for its quality-of-life features, not its paradigm. Many C++ features add cognitive overhead and do things behind your back, so code should stay plain and explicit: a reader should be able to see what it does from what is written.

## C++ features

Fine to use:
- Namespaces
- Function overloading
- The C++ standard library, when reinventing the wheel isn't worth it

Avoid everything else by default, especially features that run or decide things implicitly, such as:
- Constructors, destructors, and RAII
- Inheritance and virtual functions
- Operator overloading and implicit conversions
- Exceptions
- Heavy template use

If a C++ feature seems worth using and isn't listed as fine, ask first.

## Contain C++ usage

Keep C++ features from spreading. Using one in one place must not force the rest of the project to use C++ features too.
- Put C++ usage, including standard library types, inside the `.cpp` file that needs it, behind a C-like interface
- Headers expose plain structs and functions, not standard library types

## Project layout

Paths are relative to the project folder, `ClaudeTest/`:
- Header files go in `include/o2/`
- Implementation files go in `src/o2/`
- `include/` is on the include path, so include headers as `#include <o2/name.h>`
- Add new `.cpp` files to `ClaudeTest.vcxproj` so they get compiled

## Namespaces

Everything in these headers and their implementations goes in the `o2` namespace, so the code can be reused in later projects without name conflicts. Nested namespaces are fine, as long as they start with `o2`, such as `o2::math`.

## Memory

- Use arenas wherever possible
- Keep `malloc` to a minimum and in as few places as possible, rather than scattered through the program
- Treat `new` the same as `malloc`, and remember that standard library containers allocate on their own
