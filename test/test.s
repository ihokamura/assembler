	.intel_syntax noprefix

# main function
	.globl main
main:
	call test_mov
	call test_nop
	call test_push_pop
	call test_sub

	call test_external_text
	call test_external_data

	mov rax, 0
	ret


# test mov
test_mov:
	push rbp
	mov rbp, rsp
	sub rsp, 8

	mov ax, 16
	mov di, ax
	mov si, 16
	call assert_equal_uint16

	mov eax, 32
	mov edi, eax
	mov esi, 32
	call assert_equal_uint32

	mov rax, 64
	mov rdi, rax
	mov rsi, 64
	call assert_equal_uint64

	mov rax, rbp
	sub rax, 2
	mov word ptr [rax], 16
	mov rax, rbp
	sub rax, 2
	mov di, word ptr [rax]
	mov si, 16
	call assert_equal_uint16

	mov word ptr [rbp-2], 15
	mov di, word ptr [rbp-2]
	mov si, 15
	call assert_equal_uint16

	mov rax, rbp
	sub rax, 4
	mov dword ptr [rax], 32
	mov rax, rbp
	sub rax, 4
	mov edi, dword ptr [rax]
	mov esi, 32
	call assert_equal_uint32

	mov dword ptr [rbp-4], 31
	mov edi, dword ptr [rbp-4]
	mov esi, 31
	call assert_equal_uint32

	mov rax, rbp
	sub rax, 8
	mov qword ptr [rax], 64
	mov rax, rbp
	sub rax, 8
	mov rdi, qword ptr [rax]
	mov rsi, 64
	call assert_equal_uint64

	mov qword ptr [rbp-8], 63
	mov rdi, qword ptr [rbp-8]
	mov rsi, 63
	call assert_equal_uint64

	mov rax, 0
	pop rbp
	ret


# test nop
test_nop:
	nop

	mov rax, 0
	ret


# test push and pop
test_push_pop:
	mov rax, 64
	push rax

	mov rax, 65
	mov rdi, rax
	mov rsi, 65
	call assert_equal_uint64

	pop rax
	mov rdi, rax
	mov rsi, 64
	call assert_equal_uint64

	mov rax, 0
	ret


# test sub
test_sub:
	mov eax, 64
	mov edx, 1
	sub eax, edx
	mov edi, eax
	mov esi, 63
	call assert_equal_uint32

	mov eax, 64
	sub eax, 2
	mov edi, eax
	mov esi, 62
	call assert_equal_uint32

	mov rax, 64
	mov rdx, 1
	sub rax, rdx
	mov rdi, rax
	mov rsi, 63
	call assert_equal_uint64

	mov rax, 64
	sub rax, 2
	mov rdi, rax
	mov rsi, 62
	call assert_equal_uint64

	mov rax, 0
	ret


# test access to external text section
test_external_text:
	call test_external_function1

	mov rax, 0
	ret


# test access to external data section
test_external_data:
	mov rdi, qword ptr [rip+test_external_data_uint64]
	mov rsi, 64
	call assert_equal_uint64

	mov qword ptr [rip+test_external_data_uint64], 63
	mov rdi, 63
	call assert_external_data_uint64

	mov edi, dword ptr [rip+test_external_data_uint32]
	mov esi, 32
	call assert_equal_uint32

	mov qword ptr [rip+test_external_data_uint32], 31
	mov edi, 31
	call assert_external_data_uint32

	mov rax, 0
	ret
