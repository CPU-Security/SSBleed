all: process_1 process_2

process_1: src-process-1/process_1.c src-process-1/attack.S
	$(CC) $^ -o $@

process_2: src-process-2/process_2.c src-process-2/victim.S
	$(CC) $^ -o $@

clean:
	rm -rf process_1 process_2