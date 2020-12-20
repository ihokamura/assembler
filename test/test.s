	.intel_syntax noprefix

# main function
	.globl main
main:
	mov rdx, 42
	push rdx
	call local_func
	pop rdx
	mov rdi, rdx
	mov rsi, 42
	call assert_equal
	mov rax, rdx
	ret


# local function
local_func:
	mov rdx, 21
	mov rdi, rdx
	mov rsi, 21
	call assert_equal
	nop
	ret
