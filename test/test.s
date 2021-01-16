	.intel_syntax noprefix

	.text
# main function
	.globl main
main:
	call test_external_text
	call test_external_data

	mov rax, 0
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
