	.intel_syntax noprefix

# main function
	.globl main
main:
	call test_mov
	call test_nop
	call test_push_pop
	call test_sub

	mov rax, 0
	ret


# test mov
test_mov:
	mov rdx, 64
	mov rdi, rdx
	mov rsi, 64
	call assert_equal_uint64

	mov edx, 32
	mov edi, edx
	mov esi, 32
	call assert_equal_uint32

	ret


# test nop
test_nop:
	nop

	ret


# test push and pop
test_push_pop:
	mov rdx, 64
	push rdx

	mov rdx, 65
	mov rdi, rdx
	mov rsi, 65
	call assert_equal_uint64

	pop rdi
	mov rsi, 64
	call assert_equal_uint64

	ret


# test sub
test_sub:
	mov rdx, 64
	mov rax, 1
	sub rdx, rax
	mov rdi, rdx
	mov rsi, 63
	call assert_equal_uint64

	mov rdx, 64
	sub rdx, 2
	mov rdi, rdx
	mov rsi, 62
	call assert_equal_uint64

	mov edx, 64
	mov eax, 1
	sub edx, eax
	mov edi, edx
	mov esi, 63
	call assert_equal_uint32

	mov edx, 64
	sub edx, 2
	mov edi, edx
	mov esi, 62
	call assert_equal_uint32

	ret
