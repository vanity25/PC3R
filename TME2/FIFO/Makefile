FT_PATH := ../../ft_v1.0
CC := gcc

CFLAGS  := -I$(FT_PATH)/include -g -Wall -D_REENTRANT
LDFLAGS := -L$(FT_PATH)/lib -lfthread -lpthread

main: fifo.c
	$(CC) fifo.c $(CFLAGS) -o main $(LDFLAGS)

clean:
	rm -f main *.o
