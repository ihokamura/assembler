	.intel_syntax noprefix

# main function
	.globl main
main:
	call test_mov
	call test_nop
	call test_push_pop

	mov rax, 0
	ret


# test mov
test_mov:
	mov rdx, 42
	mov rdi, rdx
	mov rsi, 42
	call assert_equal_uint64

	ret


# test nop
test_nop:
	nop

	ret


# test push and pop
test_push_pop:
	mov rdx, 42
	push rdx

	mov rdx, 21
	mov rdi, rdx
	mov rsi, 21
	call assert_equal_uint64

	pop rdi
	mov rsi, 42
	call assert_equal_uint64

	ret
