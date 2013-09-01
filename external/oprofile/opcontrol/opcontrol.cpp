/*
 * Copyright 2008, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * Binary implementation of the original opcontrol script due to missing tools
 * like awk, test, etc.
 */

#include <unistd.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "op_config.h"

#define verbose(fmt...) if (verbose_print) printf(fmt)

/* Experiments found that using a small interval may hang the device, and the
 * more events tracked simultaneously, the longer the interval has to be.
 */

#if defined(__i386__) || defined(__x86_64__)
#define MAX_EVENTS 2
int min_count[MAX_EVENTS] = {60000, 100000};
#elif !defined(WITH_ARM_V7_A)
#define MAX_EVENTS 3
int min_count[MAX_EVENTS] = {150000, 200000, 250000};
#else
#define MAX_EVENTS 5
int min_count[MAX_EVENTS] = {150000, 20000, 25000, 30000, 35000};
#endif

int verbose_print;
int list_events; 
int show_usage;
int setup;
int quick;
int timer;
int num_events;
int start;
int stop;
int reset;

int selected_events[MAX_EVENTS];
int selected_counts[MAX_EVENTS];

char callgraph[8];
char kernel_range[512];
char vmlinux[512];

struct option long_options[] = {
    {"help", 0, &show_usage, 1},
    {"list-events", 0, &list_events, 1},
    {"reset", 0, &reset, 1},
    {"setup", 0, &setup, 1},
    {"quick", 0, &quick, 1},
    {"timer", 0, &timer, 1},
    {"callgraph", 1, 0, 'c'},
    {"event", 1, 0, 'e'},
    {"vmlinux", 1, 0, 'v'},
    {"kernel-range", 1, 0, 'r'},
    {"start", 0, &start, 1},
    {"stop", 0, &stop, 1},
    {"dump", 0, 0, 'd'},
    {"shutdown", 0, 0, 'h'},
    {"status", 0, 0, 't'},
    {"verbose", 0, 0, 'V'},
    {"verbose-log", 1, 0, 'l'},
    {0, 0, 0, 0},
};

struct event_info {
    int id;
    int um;
    const char *name;
    const char *explanation;
} event_info[] = {
#if defined(__i386__) || defined(__x86_64__)
    /* INTEL_ARCH_PERFMON events */

    /* 0x3c counters:cpuid um:zero minimum:6000 filter:0 name:CPU_CLK_UNHALTED :
     * Clock cycles when not halted
     */
    {0x3c, 0, "CPU_CLK_UNHALTED",
     "Clock cycles when not halted" },

    /* event:0x3c counters:cpuid um:one minimum:6000 filter:2 name:UNHALTED_REFERENCE_CYCLES :
     * Unhalted reference cycles
     */
    {0x3c, 1, "UNHALTED_REFERENCE_CYCLES",
      "Unhalted reference cycles" },

    /* event:0xc0 counters:cpuid um:zero minimum:6000 filter:1 name:INST_RETIRED :
     * number of instructions retired
     */
     {0xc0, 0, "INST_RETIRED",
       "number of instructions retired"},

    /* event:0x2e counters:cpuid um:x41 minimum:6000 filter:5 name:LLC_MISSES :
     * Last level cache demand requests from this core that missed the LLC
     */
     {0x2e, 0x41, "LLC_MISSES",
       "Last level cache demand requests from this core that missed the LLC"},

    /* event:0x2e counters:cpuid um:x4f minimum:6000 filter:4 name:LLC_REFS :
     * Last level cache demand requests from this core
     */
     {0x2e, 0x4f, "LLC_REFS",
      "Last level cache demand requests from this core"},

    /* event:0xc4 counters:cpuid um:zero minimum:500 filter:6 name:BR_INST_RETIRED :
     * number of branch instructions retired
     */
     {0xc4, 0, "BR_INST_RETIRED",
       "number of branch instructions retired"},

    /* event:0xc5 counters:cpuid um:zero minimum:500 filter:7 name:BR_MISS_PRED_RETIRED :
     * number of mispredicted branches retired (precise)
     */
     {0xc5, 0, "BR_MISS_PRED_RETIRED",
       "number of mispredicted branches retired (precise)"},

#elif !defined(WITH_ARM_V7_A)
    /* ARM V6 events */
    {0x00, 0, "IFU_IFETCH_MISS", 
     "number of instruction fetch misses"},
    {0x01, 0, "CYCLES_IFU_MEM_STALL", 
     "cycles instruction fetch pipe is stalled"},
    {0x02, 0, "CYCLES_DATA_STALL", 
     "cycles stall occurs for due to data dependency"},
    {0x03, 0, "ITLB_MISS", 
     "number of Instruction MicroTLB misses"},
    {0x04, 0, "DTLB_MISS", 
     "number of Data MicroTLB misses"},
    {0x05, 0, "BR_INST_EXECUTED", 
     "branch instruction executed w/ or w/o program flow change"},
    {0x06, 0, "BR_INST_MISS_PRED", 
     "branch mispredicted"},
    {0x07, 0, "INSN_EXECUTED", 
     "instructions executed"},
    {0x09, 0, "DCACHE_ACCESS", 
     "data cache access, cacheable locations"},
    {0x0a, 0, "DCACHE_ACCESS_ALL", 
     "data cache access, all locations"},
    {0x0b, 0, "DCACHE_MISS", 
     "data cache miss"},
    {0x0c, 0, "DCACHE_WB", 
     "data cache writeback, 1 event for every half cacheline"},
    {0x0d, 0, "PC_CHANGE", 
     "number of times the program counter was changed without a mode switch"},
    {0x0f, 0, "TLB_MISS", 
     "Main TLB miss"},
    {0x10, 0, "EXP_EXTERNAL", 
     "Explicit external data access"},
    {0x11, 0, "LSU_STALL", 
     "cycles stalled because Load Store request queue is full"},
    {0x12, 0, "WRITE_DRAIN", 
     "Times write buffer was drained"},
    {0xff, 0, "CPU_CYCLES", 
     "clock cycles counter"}, 
#else
    /* ARM V7 events */
    {0x00, 0, "PMNC_SW_INCR",
     "Software increment of PMNC registers"},
    {0x01, 0, "IFETCH_MISS",
     "Instruction fetch misses from cache or normal cacheable memory"},
    {0x02, 0, "ITLB_MISS",
     "Instruction fetch misses from TLB"},
    {0x03, 0, "DCACHE_REFILL",
     "Data R/W operation that causes a refill from cache or normal cacheable"
     "memory"},
    {0x04, 0, "DCACHE_ACCESS",
     "Data R/W from cache"},
    {0x05, 0, "DTLB_REFILL",
     "Data R/W that causes a TLB refill"},
    {0x06, 0, "DREAD",
     "Data read architecturally executed (note: architecturally executed = for"
     "instructions that are unconditional or that pass the condition code)"},
    {0x07, 0, "DWRITE",
     "Data write architecturally executed"},
    {0x08, 0, "INSTR_EXECUTED",
     "All executed instructions"},
    {0x09, 0, "EXC_TAKEN",
     "Exception taken"},
    {0x0A, 0, "EXC_EXECUTED",
     "Exception return architecturally executed"},
    {0x0B, 0, "CID_WRITE",
     "Instruction that writes to the Context ID Register architecturally"
     "executed"},
    {0x0C, 0, "PC_WRITE",
     "SW change of PC, architecturally executed (not by exceptions)"},
    {0x0D, 0, "PC_IMM_BRANCH",
     "Immediate branch instruction executed (taken or not)"},
    {0x0E, 0, "PC_PROC_RETURN",
     "Procedure return architecturally executed (not by exceptions)"},
    {0x0F, 0, "UNALIGNED_ACCESS",
     "Unaligned access architecturally executed"},
    {0x10, 0, "PC_BRANCH_MIS_PRED",
     "Branch mispredicted or not predicted. Counts pipeline flushes because of"
     "misprediction"},
    {0x12, 0, "PC_BRANCH_MIS_USED",
    "Branch or change in program flow that could have been predicted"},
    {0x40, 0, "WRITE_BUFFER_FULL",
     "Any write buffer full cycle"},
    {0x41, 0, "L2_STORE_MERGED",
     "Any store that is merged in L2 cache"},
    {0x42, 0, "L2_STORE_BUFF",
     "Any bufferable store from load/store to L2 cache"},
    {0x43, 0, "L2_ACCESS",
     "Any access to L2 cache"},
    {0x44, 0, "L2_CACH_MISS",
     "Any cacheable miss in L2 cache"},
    {0x45, 0, "AXI_READ_CYCLES",
     "Number of cycles for an active AXI read"},
    {0x46, 0, "AXI_WRITE_CYCLES",
     "Number of cycles for an active AXI write"},
    {0x47, 0, "MEMORY_REPLAY",
     "Any replay event in the memory subsystem"},
    {0x48, 0, "UNALIGNED_ACCESS_REPLAY",
     "Unaligned access that causes a replay"},
    {0x49, 0, "L1_DATA_MISS",
     "L1 data cache miss as a result of the hashing algorithm"},
    {0x4A, 0, "L1_INST_MISS",
     "L1 instruction cache miss as a result of the hashing algorithm"},
    {0x4B, 0, "L1_DATA_COLORING",
     "L1 data access in which a page coloring alias occurs"},
    {0x4C, 0, "L1_NEON_DATA",
     "NEON data access that hits L1 cache"},
    {0x4D, 0, "L1_NEON_CACH_DATA",
     "NEON cacheable data access that hits L1 cache"},
    {0x4E, 0, "L2_NEON",
     "L2 access as a result of NEON memory access"},
    {0x4F, 0, "L2_NEON_HIT",
     "Any NEON hit in L2 cache"},
    {0x50, 0, "L1_INST",
     "Any L1 instruction cache access, excluding CP15 cache accesses"},
    {0x51, 0, "PC_RETURN_MIS_PRED",
     "Return stack misprediction at return stack pop"
     "(incorrect target address)"},
    {0x52, 0, "PC_BRANCH_FAILED",
     "Branch prediction misprediction"},
    {0x53, 0, "PC_BRANCH_TAKEN",
     "Any predicted branch that is taken"},
    {0x54, 0, "PC_BRANCH_EXECUTED",
     "Any taken branch that is executed"},
    {0x55, 0, "OP_EXECUTED",
     "Number of operations executed"
     "(in instruction or mutli-cycle instruction)"},
    {0x56, 0, "CYCLES_INST_STALL",
     "Cycles where no instruction available"},
    {0x57, 0, "CYCLES_INST",
     "Number of instructions issued in a cycle"},
    {0x58, 0, "CYCLES_NEON_DATA_STALL",
     "Number of cycles the processor waits on MRC data from NEON"},
    {0x59, 0, "CYCLES_NEON_INST_STALL",
     "Number of cycles the processor waits on NEON instruction queue or"
     "NEON load queue"},
    {0x5A, 0, "NEON_CYCLES",
     "Number of cycles NEON and integer processors are not idle"},
    {0x70, 0, "PMU0_EVENTS",
     "Number of events from external input source PMUEXTIN[0]"},
    {0x71, 0, "PMU1_EVENTS",
     "Number of events from external input source PMUEXTIN[1]"},
    {0x72, 0, "PMU_EVENTS",
     "Number of events from both external input sources PMUEXTIN[0]"
     "and PMUEXTIN[1]"},
    {0xFF, 0, "CPU_CYCLES",
     "Number of CPU cycles"},
#endif
};

void usage()
{
    printf("\nopcontrol: usage:\n"
           "   --list-events    list event types\n"
           "   --help           this message\n"
           "   --verbose        show extra status\n"
           "   --verbose-log=lvl set daemon logging verbosity during setup\n"
           "                    levels are: all,sfile,arcs,samples,module,misc\n"
           "   --setup          setup directories\n"
#if defined(__i386__) || defined(__x86_64__)
           "   --quick          setup and select CPU_CLK_UNHALTED:60000\n"
#else
           "   --quick          setup and select CPU_CYCLES:150000\n"
#endif
           "   --timer          timer-based profiling\n"
           "   --status         show configuration\n"
           "   --start          start data collection\n"
           "   --stop           stop data collection\n"
           "   --reset          clears out data from current session\n"
           "   --shutdown       kill the oprofile daeman\n"
           "   --callgraph=depth callgraph depth\n"
           "   --event=eventspec\n"
           "      Choose an event. May be specified multiple times.\n"
           "      eventspec is in the form of name[:count], where :\n"
           "        name:  event name, see \"opcontrol --list-events\"\n"
           "        count: reset counter value\n" 
           "   --vmlinux=file   vmlinux kernel image\n"
           "   --kernel-range=start,end\n"
           "                    kernel range vma address in hexadecimal\n"
          );
}

void setup_session_dir()
{
    int fd;

    fd = open(OP_DATA_DIR, O_RDONLY);
    if (fd != -1) {
        system("rm -r "OP_DATA_DIR);
        close(fd);
    }

    if (mkdir(OP_DATA_DIR, 0755)) {
        fprintf(stderr, "Cannot create directory \"%s\": %s\n",
                OP_DATA_DIR, strerror(errno));
    }
    if (mkdir(OP_DATA_DIR"/samples", 0755)) {
        fprintf(stderr, "Cannot create directory \"%s\": %s\n",
                OP_DATA_DIR"/samples", strerror(errno));
    }
}

int read_num(const char* file)
{
    char buffer[256];
    int fd = open(file, O_RDONLY);
    if (fd<0) return -1;
    int rd = read(fd, buffer, sizeof(buffer)-1);
    buffer[rd] = 0;
    close(fd);
    return atoi(buffer);
}

int do_setup()
{
    char dir[1024];

    /*
     * Kill the old daemon so that setup can be done more than once to achieve
     * the same effect as reset.
     */
    int num = read_num(OP_DATA_DIR"/lock");
    if (num >= 0) {
        printf("Terminating the old daemon...\n");
        kill(num, SIGTERM);
        sleep(5);
    }

    setup_session_dir();

    if (mkdir(OP_DRIVER_BASE, 0755)) {
        if (errno != EEXIST) {
            fprintf(stderr, "Cannot create directory "OP_DRIVER_BASE": %s\n",
                    strerror(errno));
            return -1;
        }
    }

    if (access(OP_DRIVER_BASE"/stats", F_OK)) {
        if (system("mount -t oprofilefs nodev "OP_DRIVER_BASE)) {
            return -1;
        }
    }
    return 0;
}

void do_list_events()
{
    unsigned int i;

    printf("%-20s: %s\n", "name", "meaning");
    printf("----------------------------------------"
           "--------------------------------------\n");
    for (i = 0; i < sizeof(event_info)/sizeof(struct event_info); i++) {
        printf("%-20s: %s\n", event_info[i].name, event_info[i].explanation);
    }
}

int find_event_idx_from_name(const char *name)
{
    unsigned int i;

    for (i = 0; i < sizeof(event_info)/sizeof(struct event_info); i++) {
        if (!strcmp(name, event_info[i].name)) {
            return i;
        }
    }
    return -1;
}

const char * find_event_name_from_id(int id)
{
    unsigned int i;

    for (i = 0; i < sizeof(event_info)/sizeof(struct event_info); i++) {
        if (event_info[i].id == id) {
            return event_info[i].name;
        }
    }
    return NULL;
}

int process_event(const char *event_spec)
{
    char event_name[512];
    char count_name[512];
    unsigned int i;
    int event_idx;
    int count_val;

    strncpy(event_name, event_spec, 512);
    count_name[0] = 0;

    /* First, check if the name is followed by ":" */
    for (i = 0; i < strlen(event_name); i++) {
        if (event_name[i] == 0) {
            break;
        }
        if (event_name[i] == ':') {
            strncpy(count_name, event_name+i+1, 512);
            event_name[i] = 0;
            break;
        }
    }
    event_idx = find_event_idx_from_name(event_name);
    if (event_idx == -1) {
        fprintf(stderr, "Unknown event name: %s\n", event_name);
        return -1;
    }

    /* Use default count */
    if (count_name[0] == 0) {
        count_val = min_count[0];
    } else {
        count_val = atoi(count_name);
    }

    selected_events[num_events] = event_idx;
    selected_counts[num_events++] = count_val;
    verbose("event_id is %d\n", event_info[event_idx].id);
    verbose("count_val is %d\n", count_val);
    return 0;
}

int echo_dev(const char* str, int val, const char* file, int counter)
{
    char fullname[512];
    char content[128];
    int fd;
    
    if (counter >= 0) {
        snprintf(fullname, 512, OP_DRIVER_BASE"/%d/%s", counter, file);
    }
    else {
        snprintf(fullname, 512, OP_DRIVER_BASE"/%s", file);
    }
    fd = open(fullname, O_WRONLY);
    if (fd<0) {
        fprintf(stderr, "Cannot open %s: %s\n", fullname, strerror(errno));
        return fd;
    }
    if (str == 0) {
        sprintf(content, "%d", val);
    }
    else {
        strncpy(content, str, 128);
    }
    verbose("Configure %s (%s)\n", fullname, content);
    write(fd, content, strlen(content));
    close(fd);
    return 0;
}

void do_status()
{
    int num;
    char fullname[512];
    int i;

    printf("Driver directory: %s\n", OP_DRIVER_BASE);
    printf("Session directory: %s\n", OP_DATA_DIR);
    for (i = 0; i < MAX_EVENTS; i++) {
        sprintf(fullname, OP_DRIVER_BASE"/%d/enabled", i);
        num = read_num(fullname);
        if (num > 0) {
            printf("Counter %d:\n", i);

            /* event name */
            sprintf(fullname, OP_DRIVER_BASE"/%d/event", i);
            num = read_num(fullname);
            printf("    name: %s\n", find_event_name_from_id(num));

            /* profile interval */
            sprintf(fullname, OP_DRIVER_BASE"/%d/count", i);
            num = read_num(fullname);
            printf("    count: %d\n", num);
        }
        else {
            printf("Counter %d disabled\n", i);
        }
    }

    num = read_num(OP_DATA_DIR"/lock");
    if (num >= 0) {
        int fd;
        /* Still needs to check if this lock is left-over */
        sprintf(fullname, "/proc/%d", num);
        fd = open(fullname, O_RDONLY);
        if (fd == -1) {
            printf("OProfile daemon exited prematurely - redo setup"
                   " before you continue\n");
            return;
        }
        else {
            close(fd);

            printf("oprofiled pid: %d\n", num);
            num = read_num(OP_DRIVER_BASE"/enable");

            printf("profiler is%s running\n", num == 0 ? " not" : "");

            DIR* dir = opendir(OP_DRIVER_BASE"/stats");
            if (dir) {
                for (struct dirent* dirent; !!(dirent = readdir(dir));) {
                    if (strlen(dirent->d_name) >= 4 && memcmp(dirent->d_name, "cpu", 3) == 0) {
                        char cpupath[256];
                        strcpy(cpupath, OP_DRIVER_BASE"/stats/");
                        strcat(cpupath, dirent->d_name);

                        strcpy(fullname, cpupath);
                        strcat(fullname, "/sample_received");
                        num = read_num(fullname);
                        printf("  %s %9u samples received\n", dirent->d_name, num);

                        strcpy(fullname, cpupath);
                        strcat(fullname, "/sample_lost_overflow");
                        num = read_num(fullname);
                        printf("  %s %9u samples lost overflow\n", dirent->d_name, num);

                        strcpy(fullname, cpupath);
                        strcat(fullname, "/sample_invalid_eip");
                        num = read_num(fullname);
                        printf("  %s %9u samples invalid eip\n", dirent->d_name, num);

                        strcpy(fullname, cpupath);
                        strcat(fullname, "/backtrace_aborted");
                        num = read_num(fullname);
                        printf("  %s %9u backtrace aborted\n", dirent->d_name, num);
                    }
                }
                closedir(dir);
            }

            num = read_num(OP_DRIVER_BASE"/backtrace_depth");
            printf("backtrace_depth: %u\n", num);
        }
    }
    else {
        printf("oprofiled is not running\n");
    }
}

void do_reset() 
{
    /*
     * Sending SIGHUP will result in the following crash in oprofiled when
     * profiling subsequent runs:
     * Stack Trace:
     * RELADDR   FUNCTION                         FILE:LINE
     *   00008cd8  add_node+12                    oprofilelibdb/db_insert.c:32
     *   00008d69  odb_update_node_with_offset+60 oprofilelibdb/db_insert.c:102
     *
     * However without sending SIGHUP oprofile cannot be restarted successfully.
     * As a temporary workaround, change do_reset into a no-op for now and kill
     * the old daemon in do_setup to start all over again as a heavy-weight
     * reset.
     */
#if 0
    int fd;

    fd = open(OP_DATA_DIR"/samples/current", O_RDONLY);
    if (fd == -1) {
        return;
    }
    close(fd);
    system("rm -r "OP_DATA_DIR"/samples/current");
    int num = read_num(OP_DATA_DIR"/lock");

    if (num >= 0) {
        printf("Signalling daemon...\n");
        kill(num, SIGHUP);
    }
#endif
}

int main(int argc, char * const argv[])
{
    int option_index;
    bool show_status = false;
    char* verbose_log = NULL;

    /* Initialize default strings */
    strcpy(vmlinux, "--no-vmlinux");
    strcpy(kernel_range, "");

    while (1) {
        int c = getopt_long(argc, argv, "c:e:v:r:dhVtl:", long_options, &option_index);
        if (c == -1) {
            break;
        }
        switch (c) {
            case 0:
                break;
            /* --callgraph */
            case 'c':
                strncpy(callgraph, optarg, sizeof(callgraph));
                break;
            /* --event */
            case 'e':   
                if (num_events == MAX_EVENTS) {
                    fprintf(stderr, "More than %d events specified\n",
                            MAX_EVENTS);
                    exit(1);
                }
                if (process_event(optarg)) {
                    exit(1);
                }
                break;
            /* --vmlinux */
            case 'v':
                sprintf(vmlinux, "-k %s", optarg);
                break;
            /* --kernel-range */
            case 'r':
                sprintf(kernel_range, "-r %s", optarg);
                break;
            case 'd':
            /* --dump */ {
                int pid = read_num(OP_DATA_DIR"/lock");
                echo_dev("1", 0, "dump", -1);
                break;
            }
            /* --shutdown */
            case 'h': {
                int pid = read_num(OP_DATA_DIR"/lock");
                if (pid >= 0) {
                    kill(pid, SIGHUP); /* Politely ask the daemon to close files */
                    sleep(1);
                    kill(pid, SIGTERM);/* Politely ask the daemon to die */
                    sleep(1);
                    kill(pid, SIGKILL);
                }
                setup_session_dir();
                break;
            }
            /* --verbose */
            case 'V':
                verbose_print++;
                break;
            /* --verbose-log */
            case 'l':
                verbose_log = strdup(optarg);
                break;
            /* --status */
            case 't':
                show_status = true;
                break;
            default:
                usage();
                exit(1);
        }
    }
    verbose("list_events = %d\n", list_events);
    verbose("setup = %d\n", setup);

    if (list_events) {
        do_list_events();
    }

    if (quick) {
#if defined(__i386__) || defined(__x86_64__)
        process_event("CPU_CLK_UNHALTED");
#else
        process_event("CPU_CYCLES");
#endif
        setup = 1;
    }

    if (timer) {
        setup = 1;
    }

    if (reset) {
        do_reset();
    }

    if (show_usage) {
        usage();
    }

    if (setup) {
        if (do_setup()) {
            fprintf(stderr, "do_setup failed");
            exit(1);
        }
    }

    if (strlen(callgraph)) {
        echo_dev(callgraph, 0, "backtrace_depth", -1);
    }

    if (num_events != 0 || timer != 0) {
        char command[1024];
        int i;

        strcpy(command, argv[0]);
        char* slash = strrchr(command, '/');
        strcpy(slash ? slash + 1 : command, "oprofiled --session-dir="OP_DATA_DIR);

#if defined(__i386__) || defined(__x86_64__)
        /* Nothing */
#elif !defined(WITH_ARM_V7_A)
        /* Since counter #3 can only handle CPU_CYCLES, check and shuffle the 
         * order a bit so that the maximal number of events can be profiled
         * simultaneously
         */
        if (num_events == 3) {
            for (i = 0; i < num_events; i++) {
                int event_idx = selected_events[i];

                if (event_info[event_idx].id == 0xff) {
                    break;
                }
            }

            /* No CPU_CYCLES is found */
            if (i == 3) {
                fprintf(stderr, "You can only specify three events if one of "
                                "them is CPU_CYCLES\n");
                exit(1);
            }
            /* Swap CPU_CYCLES to counter #2 (starting from #0)*/
            else if (i != 2) {
                int temp;

                temp = selected_events[2];
                selected_events[2] = selected_events[i];
                selected_events[i] = temp;

                temp = selected_counts[2];
                selected_counts[2] = selected_counts[i];
                selected_counts[i] = temp;
            }
        }
#endif

        /* Configure the counters and enable them */
        for (i = 0; i < num_events; i++) {
            int event_idx = selected_events[i];
            int setup_result = 0;

            if (i == 0) {
                snprintf(command + strlen(command), sizeof(command) - strlen(command),
                        " --events=");
            } else {
                snprintf(command + strlen(command), sizeof(command) - strlen(command), ",");
            }
            /* Compose name:id:count:unit_mask:kernel:user, something like
             * --events=CYCLES_DATA_STALL:2:0:200000:0:1:1,....
             */
            snprintf(command + strlen(command), sizeof(command) - strlen(command),
                     "%s:%d:%d:%d:%d:1:1",
                     event_info[event_idx].name,
                     event_info[event_idx].id,
                     i,
                     selected_counts[i],
                     event_info[event_idx].um);

            setup_result |= echo_dev("1", 0, "user", i);
            setup_result |= echo_dev("1", 0, "kernel", i);
            setup_result |= echo_dev(NULL, event_info[event_idx].um, "unit_mask", i);
            setup_result |= echo_dev("1", 0, "enabled", i);
            setup_result |= echo_dev(NULL, selected_counts[i], "count", i);
            setup_result |= echo_dev(NULL, event_info[event_idx].id, 
                                     "event", i);
            if (setup_result) {
                fprintf(stderr, "Counter configuration failed for %s\n",
                        event_info[event_idx].name);
                fprintf(stderr, "Did you do \"opcontrol --setup\" first?\n");
                exit(1);
            }
        }

        if (timer == 0) {
            /* If not in timer mode, disable unused counters */
            for (i = num_events; i < MAX_EVENTS; i++) {
                echo_dev("0", 0, "enabled", i);
            }
        } else {
            /* Timer mode uses empty event list */
            snprintf(command + strlen(command), sizeof(command) - strlen(command),
                    " --events=");
        }

        snprintf(command + strlen(command), sizeof(command) - strlen(command),
                " %s", vmlinux);
        if (kernel_range[0]) {
            snprintf(command + strlen(command), sizeof(command) - strlen(command),
                    " %s", kernel_range);
        }

        if (verbose_log) {
            snprintf(command + strlen(command), sizeof(command) - strlen(command),
                    " --verbose=%s", verbose_log);
        }

        printf("Starting oprofiled...\n");
        verbose("command: %s\n", command);

        int rc = system(command);
        if (rc) {
            fprintf(stderr, "Failed, oprofile returned exit code: %d\n", rc);
        } else {
            sleep(2);
            printf("Ready\n");
        }
    }

    if (start) {
        echo_dev("1", 0, "enable", -1);
        int num = read_num(OP_DATA_DIR"/lock");

        if (num >= 0) {
            kill(num, SIGUSR1);
        }
    }

    if (stop) {
        echo_dev("1", 0, "dump", -1);
        echo_dev("0", 0, "enable", -1);
    }

    if (show_status) {
        do_status();
    }
}
