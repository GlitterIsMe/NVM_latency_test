#include <iostream>
#include <string>
#include <chrono>
#include <cstring>
#include "libpmem.h"

#define CLFLUSH

const uint64_t WRITE_TIMES = 1024 * 1024;
const uint64_t CACHELINE_SIZE = 64;
const std::string NVM_PATH = "/pmem0/zyw_test/test";

const uint WRITE_SIZE = 1 * CACHELINE_SIZE;
const uint64_t TEST_SIZE = WRITE_SIZE * WRITE_TIMES + 1024;

static uint64_t write_latency = 0;
static uint64_t CPU_FREQ_MHZ = 2100;

static inline void cpu_pause() {
    __asm__ volatile ("pause" ::: "memory");
}

static inline unsigned long read_tsc(void) {
    unsigned long var;
    unsigned int hi, lo;

    asm volatile ("rdtsc" : "=a" (lo), "=d" (hi));
    var = ((unsigned long long int) hi << 32) | lo;

    return var;
}

static inline void mfence() {
    asm volatile("mfence":::"memory");
}

static inline void clflush(char *data, int len, bool front, bool back)
{
    volatile char *ptr = (char *)((unsigned long)data &~(CACHELINE_SIZE-1));
    if (front)
        mfence();
    for(; ptr<data+len; ptr+=CACHELINE_SIZE){
        unsigned long etsc = read_tsc() + (unsigned long)(write_latency*CPU_FREQ_MHZ/1000);
#ifdef CLFLUSH
        asm volatile("clflush %0" : "+m" (*(volatile char *)ptr));
#elif CLFLUSH_OPT
        asm volatile(".byte 0x66; clflush %0" : "+m" (*(volatile char *)(ptr)));
#elif CLWB
        asm volatile(".byte 0x66; xsaveopt %0" : "+m" (*(volatile char *)(ptr)));
#endif
        while(read_tsc() < etsc) cpu_pause();
    }
    if (back)
        mfence();
}

int main() {
    int is_pmem = false;
    uint64_t mapped_len = 0;
    char* raw = (char*)pmem_map_file(NVM_PATH.c_str(), TEST_SIZE, PMEM_FILE_CREATE, 0, &mapped_len, &is_pmem);
    if (!is_pmem || mapped_len == 0) {
        std::cout << "map file error\n";
        exit(-1);
    }

    auto start = std::chrono::high_resolution_clock::now();
    for(int i = 0; i < WRITE_TIMES; i++) {
        char buf[WRITE_SIZE];
        memcpy(raw + i * WRITE_SIZE, buf, WRITE_SIZE);
        clflush(raw + i * WRITE_SIZE, WRITE_SIZE, true, true);
    }
    auto end = std::chrono::high_resolution_clock::now();
    float elapse = std::chrono::duration<float>(end - start).count();
    std::cout << "FINISH, used [" << elapse <<"]" << std::endl;
    pmem_unmap(raw, mapped_len);
    return 0;
}
