CFLAGS = -Wall -g -fsanitize=address

%: %.c
	gcc $(CFLAGS) -o $@ $^

all:
	gcc $(CFLAGS) -o mymalloc mymalloc.c

debug:
	gcc -g -Werror -Wvla -o memperf  memperf.c mymalloc.c

clean:
	rm -rf mymalloc
	rm -rf *.o