all: single_target

single_target: single_target.c driver.c inst.c
	$(CC) $^ -o $@

clean:
	rm -f single_target