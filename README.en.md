# Basm

#### Brief
An object-oriented low level language with extremely flexible syntax, currently under construction.

#### Installation

- Dependencies: `flex`, `g++`, `make`
- Compile:
```bash
make -j$(nproc)
```

#### Usage

Test cases:
```bash
./main [OPTIONS] <filename>
```
- --lex,-l: Only run lexer
- --parse,-p: Run lexer and parser
- --both,-b: Run all
