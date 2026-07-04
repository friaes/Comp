# TIL Language Compiler

A complete compiler for the **TIL** programming language, written in **C++**, built for the **Compiladores** (Compilers) course at **Instituto Superior Técnico (IST)**, University of Lisbon, 2023/24.

The compiler takes TIL source code and produces **x86 (32-bit) assembly**, which is then assembled and linked into a native executable. It's built on the course's **CDK** (Compiler Development Kit) framework and follows the classic multi-pass compiler architecture: **lexical analysis → parsing → semantic analysis → code generation**.

The full language reference is the [TIL Language Manual](https://robots.hlt.inesc-id.pt/w/pt/index.php/Compiladores/Projecto_de_Compiladores/Projecto_2023-2024/Manual_de_Refer%C3%AAncia_da_Linguagem_TIL).

## The TIL language

TIL is a small, weakly-typed, **Lisp-like language** (fully parenthesised, prefix notation) whose data model is compatible with C. It has:

- Four basic types — `int` (4-byte), `double` (8-byte IEEE 754), `string`, and `void` — plus **pointers** (`int!`, `double!!`, …) and **function-pointer types** (`(int (int))`), with pointer arithmetic and implicit int→double conversion.
- **Functions as first-class values** — anonymous functions bound to identifiers or pointers, recursion via the `@` operator, and higher-order functions (functions taking and returning function pointers).
- A single **global namespace** with `public` / `forward` / `external` linkage qualifiers for multi-module programs and C interop, and `var` for type-inferred declarations.
- Control flow: `if`, `loop`, `stop`/`next` (like `break`/`continue`, with a loop-depth argument), `return`, and `block`.
- Expression operators for arithmetic, comparison, logic (with short-circuit `&&`/`||`), assignment (`set`), pointer indexing (`index`), address-of (`?`), stack allocation (`objects`), and `sizeof`.
- I/O via `read`, `print`/`println`, and program arguments through the run-time system.

A taste of the language (recursive factorial):

```lisp
(public factorial
  (function (int (int n))
    (if (> n 1)
        (return (* n (@ (- n 1))))
        (return 1))))
```

## Architecture

The compiler is a pipeline of passes over an **abstract syntax tree**, using the **visitor pattern** (the CDK framework's `basic_ast_visitor`) so that each pass is cleanly separated from the tree structure.

### 1. Lexical analysis — `til_scanner.l` (Flex)
Turns the character stream into tokens: keywords, operators, identifiers, and literals. It handles the fiddly real-world details — **nested `/* */` comments** and line comments (via lexer start-states), **string literals** with escape sequences and octal codes, and **integer/double literals with overflow detection** (a lexical error on numbers too large for the machine).

### 2. Syntactic analysis — `til_parser.y` (Bison / LALR)
A grammar that parses TIL's prefix syntax and builds the AST through a **node factory** (`factory.h`). The grammar is **conflict-free** (no shift/reduce or reduce/reduce conflicts), which is the sign of a cleanly designed grammar.

### 3. The AST — `til/ast/*.h`
A node class per language construct: declarations, function definitions and calls, blocks, the control-flow statements (`if`, `loop`, `stop`, `next`, `return`), `print`/`read`, and the expression/pointer operators (`index`, `objects`, `sizeof`, address-of, `null`). Common node types (integers, strings, binary operators, sequences) come from CDK.

### 4. Semantic analysis — `targets/type_checker.cpp`
Walks the AST and **annotates every node with a type**, enforcing TIL's typing rules: operator operand compatibility, implicit `int`→`double` promotion, pointer and **functional-type** (covariant) compatibility, `null` compatibility with any pointer, and correct use of declarations and symbols. Type errors are reported here rather than crashing later. A `symbol.h` symbol table tracks identifiers, their types, and their scope/linkage.

### 5. Code generation — `targets/postfix_writer.cpp` (+ `frame_size_calculator.cpp`)
The final pass emits **Postfix assembly** through CDK's code-generation target: laying out global data, computing function **stack-frame sizes**, implementing the **Cdecl calling convention** (arguments pushed right-to-left, caller cleans up), local-variable offsets, control-flow labels, short-circuit evaluation, and pointer/array access. The output is `yasm`-compatible x86 assembly.

### Bonus pass — `targets/xml_writer.cpp`
Serialises the type-annotated AST to **XML**, used as the intermediate-delivery deliverable and a handy debugging view of the tree.

## Requirements

- A C++ compiler and `make`
- **Flex** and **Bison**
- **`yasm`** (assembler) and a linker (`ld`) targeting 32-bit ELF
- The course-provided **CDK** (Compiler Development Kit) and **RTS** (Run-Time System) libraries

## Building & running

With CDK/RTS available, build the compiler with `make` in the `til/` directory, then compile a TIL program end-to-end:

```bash
# TIL -> assembly
./til --target asm program.til      # produces program.asm
# assembly -> object -> executable
yasm -felf32 program.asm
ld -melf_i386 -o program program.o -lrts
./program
```

The compiler also supports `--target xml` to dump the type-annotated AST.

## Testing

The `tests/` directory holds **132 TIL programs** with their expected outputs in `tests/expected/`. `check-expected.sh` runs the full pipeline on every test — compile → assemble (`yasm`) → link (`-lrts`) → execute → diff against the expected output — under time and memory limits, and reports how many pass. `check-parser.sh` checks parsing in isolation.

```bash
./check-expected.sh    # end-to-end compile/run/diff over all 132 tests
```

## Repository layout

```
.
├── til/
│   ├── til_scanner.l          # Flex lexer
│   ├── til_parser.y           # Bison (LALR) grammar
│   ├── factory.{h,cpp}        # AST node factory
│   ├── ast/*.h                # AST node classes (one per construct)
│   ├── targets/
│   │   ├── type_checker.cpp   # semantic analysis / type annotation
│   │   ├── postfix_writer.cpp # x86 code generation
│   │   ├── frame_size_calculator.cpp
│   │   ├── xml_writer.cpp     # AST -> XML (intermediate delivery)
│   │   └── symbol.h           # symbol table entries
│   └── Makefile
├── tests/                     # 132 .til programs + expected/ outputs
├── check-expected.sh          # end-to-end test runner
└── check-parser.sh            # parser-only checker
```

## Notes

The starting point was the CDK "Simple" reference compiler with the names changed to TIL; the project consisted of building out the scanner, grammar, symbol table, type checker, XML writer (intermediate delivery), and Postfix code generator (final delivery) to implement the full TIL specification. The CDK and RTS libraries are course-provided and required to build. Developed as a two-person group project.
