# Braindead (WIP)

Braindead is a brainfuck-like programming language with some useful features. It is currently backward compatible with original
brainfuck (if your file does not contain characters other than <>+-,.[]).

It currently only compiles to x86_64 GAS.

## Syntax

Original brainfuck syntax still works.

* The '$' operator followed by an unsigned integer results in setting the current cell to it
* An unsigned integer followed by '<', '>', '+', '-' results in repeating the operator action
* Any other character except space, tab and newline result in setting the current cell to the ASCII code of that character

## Usage

Currently can append to an existing output file, too bad!

```bash
bdc <input file> <output file>
```
