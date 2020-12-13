	.intel_syntax noprefix

# main function
	.globl main
main:
	call local_func
	nop
	mov rax, rdx
	ret


# local function
local_func:
	mov rdx, 42
	ret
