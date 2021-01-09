CC=gcc
CFLAGS=-std=c11 -g -Wall

ASM_SRCS=$(wildcard source/*.c)
ASM_HDRS=$(wildcard source/*.h)
ASM_OBJS=$(ASM_SRCS:.c=.o)
ASM_BIN=asm

TEST_GENERATOR_SRCS=$(wildcard test/generator/*.c)
TEST_GENERATOR_HDRS=$(wildcard test/generator/*.h)
TEST_GENERATOR_OBJS=$(TEST_GENERATOR_SRCS:.c=.o)
TEST_GENERATOR_BIN=test/test_generator

TEST_SH=test/test.sh

asm: $(ASM_OBJS)
	$(CC) $(ASM_OBJS) -o $(ASM_BIN) $(CFLAGS)

$(ASM_OBJS): $(ASM_HDRS)

test_generator: $(TEST_GENERATOR_OBJS)
	$(CC) $(TEST_GENERATOR_OBJS) -o $(TEST_GENERATOR_BIN) $(CFLAGS)
	$(TEST_GENERATOR_BIN)

$(TEST_GENERATOR_OBJS): $(TEST_GENERATOR_HDRS)

test: asm test_generator
	bash $(TEST_SH) ../$(ASM_BIN) $(ASM_BIN)

clean:
	rm -f $(ASM_BIN) source/*.o
	rm -f test/test_generator test/generator/*.o test/test_mov.s test/*.o test/*bin*


.PHONY: asm test test_generator clean
