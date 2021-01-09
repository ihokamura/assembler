	.intel_syntax noprefix

# main function
	.globl main
main:
	call test_push_pop

	call test_external_text
	call test_external_data

	mov rax, 0
	ret


# test push and pop
test_push_pop:
	push rbp
	mov rbp, rsp
	sub rsp, 8

	push 0x08
	pop rsi
	mov rdi, 0x08
	call assert_equal_uint8

	push 0x1616
	pop rsi
	mov rdi, 0x1616
	call assert_equal_uint16

	push 0x32323232
	pop rsi
	mov rdi, 0x32323232
	call assert_equal_uint64

	mov ax, 16
	push ax

	mov ax, 15
	mov si, ax
	mov di, 15
	call assert_equal_uint16

	pop ax
	mov si, ax
	mov di, 16
	call assert_equal_uint16

	mov r8w, 16
	push r8w

	mov r8w, 15
	mov si, r8w
	mov di, 15
	call assert_equal_uint16

	pop r8w
	mov si, r8w
	mov di, 16
	call assert_equal_uint16

	mov rax, rbp
	sub rax, 2
	mov word ptr [rax], 16
	push word ptr [rax]

	mov word ptr [rax], 15
	mov si, word ptr [rax]
	mov di, 15
	call assert_equal_uint16

	pop word ptr [rbp-2]
	mov si, word ptr [rbp-2]
	mov di, 16
	call assert_equal_uint16

	mov rax, 64
	push rax

	mov rax, 63
	mov rsi, rax
	mov rdi, 63
	call assert_equal_uint64

	pop rax
	mov rsi, rax
	mov rdi, 64
	call assert_equal_uint64

	mov r8, 64
	push r8

	mov r8, 63
	mov rsi, r8
	mov rdi, 63
	call assert_equal_uint64

	pop r8
	mov rsi, r8
	mov rdi, 64
	call assert_equal_uint64

	mov rax, rbp
	sub rax, 8
	mov qword ptr [rax], 64
	push qword ptr [rax]

	mov qword ptr [rax], 63
	mov rsi, qword ptr [rax]
	mov rdi, 63
	call assert_equal_uint64

	pop qword ptr [rbp-8]
	mov rsi, qword ptr [rbp-8]
	mov rdi, 64
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
