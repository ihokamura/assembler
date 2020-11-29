CFLAGS=-std=c11 -g -Wall
SRCS=$(wildcard ./source/*.c)
HDRS=$(wildcard ./source/*.h)
OBJS=$(SRCS:.c=.o)


asm: $(OBJS)
	$(CC) -o asm $(OBJS) $(LDFLAGS)

$(OBJS): $(HDRS)

test: asm
	bash ./test/test.sh

clean:
	rm -f ./asm ./source/*.o ./test/test_bin test/*.o


.PHONY: asm test clean
