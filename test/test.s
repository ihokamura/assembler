	.intel_syntax noprefix

	.data
test_data_uint8:
	.byte 0xff
	.align 2
test_data_uint16:
	.word 0xffff
	.align 4
test_data_uint32:
	.long 0xffffffff
	.align 8
test_data_uint64:
	.quad 0xffffffffffffffff
test_data_pointer_to_internal_data_uint8:
	.quad test_data_uint8
test_data_pointer_to_external_data_uint8:
	.quad test_external_data_uint8

	.bss
test_bss_uint8:
	.zero 1
	.align 2
test_bss_uint16:
	.zero 2
	.align 4
test_bss_uint32:
	.zero 4
	.align 8
test_bss_uint64:
	.zero 8

	.text
# main function
	.globl main
main:
	call test_external_text
	call test_internal_data
	call test_external_data
	call test_internal_bss

	mov rax, 0
	ret


# test access to external text section
	.globl test_external_text
test_external_text:
	call test_external_function1

	mov rax, 0
	ret


# test access to internal data section
test_internal_data:
	mov sil, byte ptr [rip+test_data_uint8]
	mov dil, 0xff
	call assert_equal_uint8

	mov sil, byte ptr [rip+test_data_uint8_array]
	mov dil, 0x81
	call assert_equal_uint8

	mov sil, byte ptr [rip+test_data_uint8_array+1]
	mov dil, 0x93
	call assert_equal_uint8

	mov si, word ptr [rip+test_data_uint16]
	mov di, 0xffff
	call assert_equal_uint16

	mov si, word ptr [rip+test_data_uint16_array]
	mov di, 0x8001
	call assert_equal_uint16

	mov si, word ptr [rip+test_data_uint16_array+2]
	mov di, 0x9003
	call assert_equal_uint16

	mov esi, dword ptr [rip+test_data_uint32]
	mov edi, 0xffffffff
	call assert_equal_uint32

	mov esi, dword ptr [rip+test_data_uint32_array]
	mov edi, 0x80000001
	call assert_equal_uint32

	mov esi, dword ptr [rip+test_data_uint32_array+4]
	mov edi, 0x90000003
	call assert_equal_uint32

	mov rsi, qword ptr [rip+test_data_uint64]
	mov rdi, 0xffffffffffffffff
	call assert_equal_uint64

	mov rsi, qword ptr [rip+test_data_uint64_array]
	mov rdi, 0x8000000000000001
	call assert_equal_uint64

	mov rsi, qword ptr [rip+test_data_uint64_array+8]
	mov rdi, 0x9000000000000003
	call assert_equal_uint64

	mov rsi, qword ptr [rip+test_data_pointer_to_internal_data_uint8]
	lea rdi, byte ptr [rip+test_data_uint8]
	call assert_equal_uint64

	mov rdi, qword ptr [rip+test_data_pointer_to_external_data_uint8]
	call assert_pointer_to_external_data_uint8

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

# test access to internal bss section
test_internal_bss:
	mov sil, byte ptr [rip+test_bss_uint8]
	mov dil, 0
	call assert_equal_uint8

	mov si, word ptr [rip+test_bss_uint16]
	mov di, 0
	call assert_equal_uint16

	mov esi, dword ptr [rip+test_bss_uint32]
	mov edi, 0
	call assert_equal_uint32

	mov rsi, qword ptr [rip+test_bss_uint64]
	mov rdi, 0
	call assert_equal_uint64

	mov rax, 0
	ret

.data
test_data_uint8_array:
	.byte 0x81
	.byte 0x93
	.align 2
test_data_uint16_array:
	.word 0x8001
	.word 0x9003
	.align 4
test_data_uint32_array:
	.long 0x80000001
	.long 0x90000003
	.align 8
test_data_uint64_array:
	.quad 0x8000000000000001
	.quad 0x9000000000000003
