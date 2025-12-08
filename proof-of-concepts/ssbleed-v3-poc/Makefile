all: poc

poc: asm.S ssbleed-v3-poc.c
	$(CC) $^ -o $@

clean:
	rm -rf poc