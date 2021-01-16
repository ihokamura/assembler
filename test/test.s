	.intel_syntax noprefix

	.data
test_internal_data_uint8:
	.byte 0xff
test_internal_data_uint16:
	.word 0xffff
test_internal_data_uint32:
	.long 0xffffffff
test_internal_data_uint64:
	.quad 0xffffffffffffffff

	.text
# main function
	.globl main
main:
	call test_external_text
	call test_internal_data
	call test_external_data

	mov rax, 0
	ret


# test access to external text section
test_external_text:
	call test_external_function1

	mov rax, 0
	ret


# test access to internal data section
test_internal_data:
	mov sil, byte ptr [rip+test_internal_data_uint8]
	mov dil, 0xff
	call assert_equal_uint8

	mov sil, byte ptr [rip+test_internal_data_uint8_2]
	mov dil, 0x81
	call assert_equal_uint8

	mov si, word ptr [rip+test_internal_data_uint16]
	mov di, 0xffff
	call assert_equal_uint16

	mov si, word ptr [rip+test_internal_data_uint16_2]
	mov di, 0x8001
	call assert_equal_uint16

	mov esi, dword ptr [rip+test_internal_data_uint32]
	mov edi, 0xffffffff
	call assert_equal_uint32

	mov esi, dword ptr [rip+test_internal_data_uint32_2]
	mov edi, 0x80000001
	call assert_equal_uint32

	mov rsi, qword ptr [rip+test_internal_data_uint64]
	mov rdi, 0xffffffffffffffff
	call assert_equal_uint64

	mov rsi, qword ptr [rip+test_internal_data_uint64_2]
	mov rdi, 0x8000000000000001
	call assert_equal_uint64

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

.data
test_internal_data_uint8_2:
	.byte 0x81
test_internal_data_uint16_2:
	.word 0x8001
test_internal_data_uint32_2:
	.long 0x80000001
test_internal_data_uint64_2:
	.quad 0x8000000000000001
