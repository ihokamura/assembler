CFLAGS=-std=c11 -g -Wall
SRCS=$(wildcard ./source/*.c)
HDRS=$(wildcard ./source/*.h)
OBJS=$(SRCS:.c=.o)


asm: $(OBJS)
	$(CC) -o asm $(OBJS) $(LDFLAGS)

$(OBJS): $(HDRS)

test: asm
	bash ./test/test.sh ../asm asm

clean:
	rm -f ./asm ./source/*.o ./test/*bin* test/*.o


.PHONY: asm test clean
