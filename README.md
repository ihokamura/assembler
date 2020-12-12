# Overview
* This is a personal project to make an assembler for Intel 64 and IA-32 architecture processors.

## Usage
```
asm <input-file> -c -o <output-file>
```

## Syntax

```
program ::= statement*
statement ::= directive
            | symbol ":"
            | operation
directive ::= ".intel_syntax noprefix"
            | ".globl" symbol
operation ::= mnemonic operands?
mnemonic ::= "mov"
           | "ret"
operands ::= operand ("," operand)?
operand ::= immediate | register
register ::= "rax"
```

## Reference
[1] [Intel® 64 and IA-32 Architectures Software Developer’s Manual](
https://software.intel.com/content/www/us/en/develop/download/intel-64-and-ia-32-architectures-sdm-combined-volumes-1-2a-2b-2c-2d-3a-3b-3c-3d-and-4.html)
