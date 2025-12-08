all: existence state_machine table_index timing_difference

CFLAGS =
ifdef SAMPLED_LD_NUM
	CFLAGS += -DSAMPLED_LD_NUM=$(SAMPLED_LD_NUM)
endif

existence: src/existence.c
	$(CC) $^ -o bin/$@

state_machine: src/state_machine.c
	$(CC) $^ -o bin/$@

table_index: src/table_index.c
	$(CC) $^ -o bin/$@

table_org: src/table_org.c src/asm.S
	$(CC) $^ -o bin/$@ $(CFLAGS)

replacement_policy: src/replacement_policy.c src/asm.S
	$(CC) $^ -o bin/$@

timing_difference: src/timing_difference.c 
	$(CC) $^ -o bin/$@

clean:
	rm -rf bin/* data/*