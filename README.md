# Overview
* This is a personal project to make an assembler for Intel 64 and IA-32 architecture processors.

## Usage
```
asm <input-file> -c -o <output-file>
```

## Syntax

```
program ::= statement*
statement ::= (label ":")? directive | operation
directive ::= ".bss"
            | ".byte"
            | ".data"
            | ".globl" symbol
            | ".intel_syntax noprefix"
            | ".long"
            | ".quad"
            | ".text"
            | ".word"
            | ".zero"
operation ::= mnemonic operands?
mnemonic ::= "add"
           | "and"
           | "call"
           | "mov"
           | "nop"
           | "or"
           | "pop"
           | "push"
           | "ret"
           | "sub"
           | "xor"
operands ::= operand ("," operand)?
operand ::= immediate | register | memory | symbol
register ::= "al" | "dl" | "cl" | "bl" | "spl" | "bpl" | "sil" | "dil"
           | "ax" | "dx" | "cx" | "bx" | "sp" | "bp" | "si" | "di"
           | "eax" | "edx" | "ecx" | "ebx" | "esp" | "ebp" | "esi" | "edi"
           | "rax" | "rdx" | "rcx" | "rbx" | "rsp" | "rbp" | "rsi" | "rdi" | "rip"
memory ::= size-specifier "[" register (("+" | "-") immediate | "+" symbol)? "]"
size-specifier ::= "byte ptr" | "word ptr" | "dword ptr" | "qword ptr"
```

## Reference
[1] [Intel® 64 and IA-32 Architectures Software Developer’s Manual](
https://software.intel.com/content/www/us/en/develop/download/intel-64-and-ia-32-architectures-sdm-combined-volumes-1-2a-2b-2c-2d-3a-3b-3c-3d-and-4.html)
