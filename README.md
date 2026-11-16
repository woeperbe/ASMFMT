# asm files Formater

Lines up the columns of 8080 / 8085 / Z80 / M80 / asmx assembler source files:

```
label:  opcode  operands                ; comment
```

Only blanks are changed, so the formatted file assembles to exactly the same bytes as the original.

**Before**

```
DPE&DN:  DW XLT&DN,0000H ;Translate table
   DW   0000H,0000H;Scratch area
LAB&DNN:DS SPACE
```

**After**

```
DPE&DN: DW      XLT&DN,0000H    ;Translate table
        DW      0000H,0000H     ;Scratch area
LAB&DNN:        DS      SPACE
```

## Build

One C file, no dependencies. It compiles as C or C++.

```
cc -O2 -o asmfmt asmfmt.c
```

Visual Studio: add `asmfmt.c` (or rename it to `.cpp`) to a console project and build.

## Usage

```
asmfmt [options] file...
```

The result goes to standard output unless `-i` or `-o` is given.

| Option | Meaning | Default |
| --- | --- | --- |
| `-t` | use tabs | on |
| `-s` | use spaces instead of tabs | |
| `-w n` | tab width | 8 |
| `-c o,a,c` | columns for opcode, operands, comment | 8,16,32 |
| `-u` | uppercase opcodes | |
| `-i` | rewrite files in place, keep the original as `file.bak` | |
| `-o file` | write to this file (one input file only) | |

Examples:

```
asmfmt BIOS.MAC > NEW.MAC          # formatted copy
asmfmt -i *.MAC                    # rewrite in place, keep .bak files
asmfmt -s -c 8,16,40 BIOS.MAC      # spaces, comments at column 40
```

## What it leaves alone

- comment lines (`;` or `*` in column 1, or blanks followed by `;`)
- text in quotes (`DB "a;b",0`) and M80 `<...>` brackets (`DDW %X,<;comment>`)
- operand and comment text, character for character
- Z80 `EX AF,AF'` (an unmatched quote is just a character)
- CR/LF or LF line endings
- everything from the CP/M end-of-file mark (`^Z`) on

## Notes

- Anything starting in column 1 is treated as a label. A directive written in column 1 (`.XLIST`, `END`) stays there.
- If a field reaches the next column, a single tab or space is added instead.
- Running asmfmt twice gives the same result as running it once.

To check a formatted file, assemble both versions and compare the output:

```
asmx -C 8080 -o OLD.HEX BIOS.MAC
asmx -C 8080 -o NEW.HEX NEW.MAC
cmp OLD.HEX NEW.HEX
```
