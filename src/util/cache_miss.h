#pragma once

#include <cstdint>
#include <cstring>
#include <iostream>
#include <linux/perf_event.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <unistd.h>

class MemoryCostCounter {
    int fd_cache_miss = -1;
    int fd_dtlb_miss = -1;

    long long cache_misses = 0;
    long long dtlb_misses = 0;

    static long perf_event_open(struct perf_event_attr *hw_event, pid_t pid,
                                int cpu, int group_fd, unsigned long flags) {
        return syscall(__NR_perf_event_open, hw_event, pid, cpu, group_fd,
                       flags);
    }

    static int open_event(perf_type_id type, uint64_t config) {
        perf_event_attr pe{};
        pe.type = type;
        pe.size = sizeof(perf_event_attr);
        pe.config = config;
        pe.disabled = 1;
        pe.exclude_kernel = 1;
        pe.exclude_hv = 1;

        int fd = perf_event_open(&pe, 0, -1, -1, 0);
        if (fd == -1) {
            perror("perf_event_open");
            exit(1);
        }

        return fd;
    }

  public:
    MemoryCostCounter() {
        // Generic hardware cache misses (fallback for RAM access)
        fd_cache_miss =
            open_event(PERF_TYPE_HARDWARE, PERF_COUNT_HW_CACHE_MISSES);

        // DTLB read miss
        uint64_t dtlb_config = PERF_COUNT_HW_CACHE_DTLB |
                               (PERF_COUNT_HW_CACHE_OP_READ << 8) |
                               (PERF_COUNT_HW_CACHE_RESULT_MISS << 16);
        fd_dtlb_miss = open_event(PERF_TYPE_HW_CACHE, dtlb_config);
    }

    void start() {
        ioctl(fd_cache_miss, PERF_EVENT_IOC_RESET, 0);
        ioctl(fd_dtlb_miss, PERF_EVENT_IOC_RESET, 0);

        ioctl(fd_cache_miss, PERF_EVENT_IOC_ENABLE, 0);
        ioctl(fd_dtlb_miss, PERF_EVENT_IOC_ENABLE, 0);
    }

    void stop() {
        ioctl(fd_cache_miss, PERF_EVENT_IOC_DISABLE, 0);
        ioctl(fd_dtlb_miss, PERF_EVENT_IOC_DISABLE, 0);

        read(fd_cache_miss, &cache_misses, sizeof(long long));
        read(fd_dtlb_miss, &dtlb_misses, sizeof(long long));
    }

    long long getRAMAccesses() const { return cache_misses; }
    long long getDTLBMisses() const { return dtlb_misses; }

    void print() const {
        std::cout << "RAM accesses (generic cache misses): " << cache_misses
                  << "\n";
        std::cout << "DTLB misses: " << dtlb_misses << "\n";
    }

    ~MemoryCostCounter() {
        if (fd_cache_miss != -1)
            close(fd_cache_miss);
        if (fd_dtlb_miss != -1)
            close(fd_dtlb_miss);
    }
};
