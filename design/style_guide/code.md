# Style Guide for Code

<!-- Table of Contents only links to level 2 headers -->
\[ [Header Guards](#header-guards) \]
\[ [Function Names](#function-names) \]
\[ [Memory Ownership](#memory-ownership) \]

## Header Guards

All C code in src/ should use traditional guard style with the file path
(excluding src prefix) in upper snake case separated with underscores. A
comment should be included on the `#endif` line with the same name.

```c
#ifndef PATH_UPPER_SNAKE_CASE_H
#define PATH_UPPER_SNAKE_CASE_H
...
#endif // PATH_UPPER_SNAKE_CASE_H
```

All C++ code or C code outside src/ should use `#pragma once`

```c++
#pragma once
...
```

## Function Names

Function names should include the module (ie. path) as a prefix using snake
case.

## Memory Ownership

TODO

