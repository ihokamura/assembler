	.intel_syntax noprefix

# main function
	.globl main
main:
	call local_func
	nop
	mov rdi, rdx
	mov rsi, 42
	call assert_equal
	mov rax, rdx
	ret


# local function
local_func:
	mov rdx, 42
	ret
