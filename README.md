# MiniCompiler

A small educational compiler written in C++ that demonstrates the main stages of a compiler pipeline: lexing, parsing, semantic checks, optimization, intermediate representation (IR) generation, C code generation, and native compilation.

The project is designed to make compiler concepts easier to explore through a small source language and inspectable intermediate output.

## Features

- **Lexing and parsing** using a recursive-descent parser.
- **Abstract Syntax Tree (AST)** to represent parsed programs.
- **Semantic checks** for undeclared variables and duplicate declarations.
- **Compile-time error detection** for division by a literal zero.
- **AST constant folding** for supported constant expressions.
- **Intermediate representation (IR)** generation.
- **IR optimization**, including constant folding/propagation and dead temporary elimination.
- **C code generation** from the compiler's output.
- **Automated tests** for normal programs, optimization behavior, and selected diagnostics.

> This is a learning project, not a production compiler. Its supported language and optimization rules are intentionally limited.

## Compiler pipeline

```text
Source program (.mc)
        |
        v
      Lexer
        |
        v
 Recursive-descent parser
        |
        v
 Abstract Syntax Tree (AST)
        |
        v
 Semantic checks
        |
        v
 AST constant folding
        |
        v
 Unoptimized intermediate representation
        |
        v
 IR optimization
        |
        v
 Optimized intermediate representation
        |
        v
      C code
        |
        v
      GCC
        |
        v
 Executable program
```

The compiler writes both unoptimized and optimized IR so you can compare how the optimization stage changes the program.

## Requirements

On Windows, using PowerShell:

- A C++ compiler that supports C++17 (for example, `g++` from MinGW-w64).
- GCC to compile the generated C code.
- PowerShell.

Make sure `g++` and `gcc` are available on your `PATH`. Check with:

```powershell
g++ --version
gcc --version
```

## Build

Run these commands from the project root directory:

```powershell
g++ -std=c++17 -Wall -Wextra -Wpedantic src/main.cpp src/ir.cpp -o minic.exe
```

This compiles the MiniCompiler itself. The warning flags help surface common C++ issues during development.

## Compile and run an example

Compile the included basic example:

```powershell
.\minic.exe examples/basic.mc output.c
```

The compiler should report that compilation succeeded and identify the generated C and IR files.

Compile the generated C program with GCC:

```powershell
gcc output.c -o program.exe
```

Run it:

```powershell
.\program.exe
```

Expected output:

```text
50
15
14
```

### Inspect the intermediate representation

The compiler also creates these files for the example:

- `output.unoptimized.ir` — IR before IR-level optimization.
- `output.ir` — optimized IR.
- `output.c` — generated C source.

The optimized IR for the basic example is expected to resemble:

```text
x = 50
y = 15
print 50
print 15
print 14
```

The exact temporary names and formatting may depend on the implementation. These files are useful for learning how expressions are translated and simplified.

## Run the automated tests

From the project root, run:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\tests\run_tests.ps1
```

The current test script checks:

1. The compiler builds successfully.
2. The basic example compiles, and its generated program prints `50`, `15`, and `14`.
3. The optimization example compiles, and its generated program prints `42`, `50`, and `100`.
4. A division-by-zero input produces the expected compile-time diagnostic.
5. An undeclared-variable input produces the expected semantic diagnostic.

A successful run ends with:

```text
All tests passed!
```

The tests cover selected examples and error cases; they do not prove that every possible input is handled correctly.

## Project layout

```text
MiniCompiler_complete/
├── src/
│   ├── main.cpp
│   ├── ast.h
│   ├── ir.cpp
│   └── ir.h
├── examples/
│   ├── basic.mc
│   ├── optimization.mc
│   ├── edge_cases.mc
│   └── errors_undeclared.mc
├── tests/
│   └── run_tests.ps1
└── README.md
```

The filenames above describe the current project layout. If your local folder contains additional examples or source files, keep them in the repository as appropriate.

## Learning goals

This project provides hands-on practice with:

- How source code is tokenized and parsed.
- How an AST represents expressions and statements.
- How semantic checks catch certain invalid programs.
- Why compilers use an intermediate representation.
- How constant folding and propagation simplify computations.
- How dead temporary elimination can remove unnecessary intermediate assignments.
- How a compiler can generate C code for a separate compiler to build.

## Current limitations

- The source language is a small educational language, not full C++.
- Semantic analysis currently covers selected errors, including undeclared variables and duplicate declarations.
- Division-by-zero detection is limited to the supported compile-time case of a literal zero divisor.
- Optimizations are limited to the rules implemented in the current source.
- The automated test suite covers representative cases, not exhaustive correctness or performance.

## Possible future improvements

Ideas for future development include:

- Common subexpression elimination.
- More semantic checks and clearer source-location diagnostics.
- Additional unit and regression tests.
- Better IR visualization and optimization statistics.
- More language constructs and types.
- An LLVM or MLIR-based backend as a separate learning milestone.

These are future ideas, not features claimed to be implemented already.

## License

No license has been specified yet. If you plan to publish this project publicly, choose a license and add a `LICENSE` file before inviting others to reuse the code.
