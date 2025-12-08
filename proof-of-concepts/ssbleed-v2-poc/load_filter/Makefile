all: load_generator

load_generator: load_generator.c driver.c inst.c
	$(CC) $^ -o $@

clean:
	rm -f load_generator