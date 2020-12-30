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
	sub rsp, 16

	mov al, 8
	mov sil, al
	mov dil, 8
	call assert_equal_uint8

	mov ax, 16
	mov si, ax
	mov di, 16
	call assert_equal_uint16

	mov eax, 32
	mov esi, eax
	mov edi, 32
	call assert_equal_uint32

	mov rax, 64
	mov rsi, rax
	mov rdi, 64
	call assert_equal_uint64

	mov rax, rbp
	sub rax, 1
	mov byte ptr [rax], 8
	mov rax, rbp
	sub rax, 1
	mov sil, byte ptr [rax]
	mov dil, 8
	call assert_equal_uint8

	mov byte ptr [rbp-1], 7
	mov sil, byte ptr [rbp-1]
	mov dil, 7
	call assert_equal_uint8

	mov rax, rbp
	sub rax, 2
	mov word ptr [rax], 16
	mov rax, rbp
	sub rax, 2
	mov si, word ptr [rax]
	mov di, 16
	call assert_equal_uint16

	mov word ptr [rbp-2], 15
	mov si, word ptr [rbp-2]
	mov di, 15
	call assert_equal_uint16

	mov rax, rbp
	sub rax, 4
	mov dword ptr [rax], 32
	mov rax, rbp
	sub rax, 4
	mov esi, dword ptr [rax]
	mov edi, 32
	call assert_equal_uint32

	mov dword ptr [rbp-4], 31
	mov esi, dword ptr [rbp-4]
	mov edi, 31
	call assert_equal_uint32

	mov rax, rbp
	sub rax, 8
	mov qword ptr [rax], 64
	mov rax, rbp
	sub rax, 8
	mov rsi, qword ptr [rax]
	mov rdi, 64
	call assert_equal_uint64

	mov qword ptr [rbp-8], 63
	mov rsi, qword ptr [rbp-8]
	mov rdi, 63
	call assert_equal_uint64

	mov rax, 0
	mov rsp, rbp
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
	mov rsi, rax
	mov rdi, 65
	call assert_equal_uint64

	pop rax
	mov rsi, rax
	mov rdi, 64
	call assert_equal_uint64

	mov rax, 0
	ret


# test sub
test_sub:
	push rbp
	mov rbp, rsp
	sub rsp, 16

	mov ax, 16
	mov dx, 1
	sub ax, dx
	mov si, ax
	mov di, 15
	call assert_equal_uint16

	mov ax, 16
	sub ax, 2
	mov si, ax
	mov di, 14
	call assert_equal_uint16

	mov ax, 0x1616
	sub ax, 0x1600
	mov si, ax
	mov di, 0x16
	call assert_equal_uint16

	mov dx, 0x1616
	sub dx, 0x1601
	mov si, dx
	mov di, 0x15
	call assert_equal_uint16

	mov eax, 32
	mov edx, 1
	sub eax, edx
	mov esi, eax
	mov edi, 31
	call assert_equal_uint32

	mov eax, 32
	sub eax, 2
	mov esi, eax
	mov edi, 30
	call assert_equal_uint32

	mov eax, 0x32323232
	sub eax, 0x32323200
	mov esi, eax
	mov edi, 0x32
	call assert_equal_uint32

	mov edx, 0x32323232
	sub edx, 0x32323201
	mov esi, edx
	mov edi, 0x31
	call assert_equal_uint32

	mov rax, rbp
	sub rax, 4
	mov dword ptr [rax], 32
	sub dword ptr [rax], 3
	mov esi, dword ptr [rax]
	mov edi, 29
	call assert_equal_uint64

	mov rax, 64
	mov rdx, 1
	sub rax, rdx
	mov rsi, rax
	mov rdi, 63
	call assert_equal_uint64

	mov rax, 64
	sub rax, 2
	mov rsi, rax
	mov rdi, 62
	call assert_equal_uint64

	mov rax, 0x64646464
	sub rax, 0x64646400
	mov rsi, rax
	mov rdi, 0x64
	call assert_equal_uint64

	mov rdx, 0x64646464
	sub rdx, 0x64646401
	mov rsi, rdx
	mov rdi, 0x63
	call assert_equal_uint64

	mov rax, rbp
	sub rax, 8
	mov qword ptr [rax], 64
	sub qword ptr [rax], 3
	mov rsi, qword ptr [rax]
	mov rdi, 61
	call assert_equal_uint64

	mov rax, 0
	mov rsp, rbp
	pop rbp
	ret


# test access to external text section
test_external_text:
	call test_external_function1

	mov rax, 0
	ret


# test access to external data section
test_external_data:
	mov sil, byte ptr [rip+test_external_data_uint8]
	mov dil, 8
	call assert_equal_uint8

	mov byte ptr [rip+test_external_data_uint8], 7
	mov dil, 7
	call assert_external_data_uint8

	mov si, word ptr [rip+test_external_data_uint16]
	mov di, 16
	call assert_equal_uint16

	mov word ptr [rip+test_external_data_uint16], 15
	mov di, 15
	call assert_external_data_uint16

	mov esi, dword ptr [rip+test_external_data_uint32]
	mov edi, 32
	call assert_equal_uint32

	mov dword ptr [rip+test_external_data_uint32], 31
	mov edi, 31
	call assert_external_data_uint32

	mov rsi, qword ptr [rip+test_external_data_uint64]
	mov rdi, 64
	call assert_equal_uint64

	mov qword ptr [rip+test_external_data_uint64], 63
	mov rdi, 63
	call assert_external_data_uint64

	mov rax, 0
	ret
