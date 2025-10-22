#include <string.h>
#include "debug.h"
#include "kernel/calls.h"
#include "emu/interrupt.h"
#include "kernel/memory.h"
#include "kernel/signal.h"
#include "kernel/task.h"

dword_t syscall_stub(void) {
    return _ENOSYS;
}
// While identical, this version of the stub doesn't log below. Use this for
// syscalls that are optional (i.e. fallback on something else) but called
// frequently.
dword_t syscall_silent_stub(void) {
    return _ENOSYS;
}
dword_t syscall_success_stub(void) {
    return 0;
}

#if is_gcc(8)
#pragma GCC diagnostic ignored "-Wcast-function-type"
#endif
syscall_t syscall_table[] = {
    [1]   = (syscall_t) sys_exit,
    [2]   = (syscall_t) sys_fork,
    [3]   = (syscall_t) sys_read,
    [4]   = (syscall_t) sys_write,
    [5]   = (syscall_t) sys_open,
    [6]   = (syscall_t) sys_close,
    [7]   = (syscall_t) sys_waitpid,
    [9]   = (syscall_t) sys_link,
    [10]  = (syscall_t) sys_unlink,
    [11]  = (syscall_t) sys_execve,
    [12]  = (syscall_t) sys_chdir,
    [13]  = (syscall_t) sys_time,
    [14]  = (syscall_t) sys_mknod,
    [15]  = (syscall_t) sys_chmod,
    [19]  = (syscall_t) sys_lseek,
    [20]  = (syscall_t) sys_getpid,
    [21]  = (syscall_t) sys_mount,
    [23]  = (syscall_t) sys_setuid,
    [24]  = (syscall_t) sys_getuid,
    [25]  = (syscall_t) sys_stime,
    [26]  = (syscall_t) sys_ptrace,
    [27]  = (syscall_t) sys_alarm,
    [29]  = (syscall_t) sys_pause,
    [30]  = (syscall_t) sys_utime,
    [33]  = (syscall_t) sys_access,
    [36]  = (syscall_t) syscall_success_stub, // sync
    [37]  = (syscall_t) sys_kill,
    [38]  = (syscall_t) sys_rename,
    [39]  = (syscall_t) sys_mkdir,
    [40]  = (syscall_t) sys_rmdir,
    [41]  = (syscall_t) sys_dup,
    [42]  = (syscall_t) sys_pipe,
    [43]  = (syscall_t) sys_times,
    [45]  = (syscall_t) sys_brk,
    [46]  = (syscall_t) sys_setgid,
    [47]  = (syscall_t) sys_getgid,
    [49]  = (syscall_t) sys_geteuid,
    [50]  = (syscall_t) sys_getegid,
    [52]  = (syscall_t) sys_umount2,
    [54]  = (syscall_t) sys_ioctl,
    [55]  = (syscall_t) sys_fcntl32,
    [57]  = (syscall_t) sys_setpgid,
    [60]  = (syscall_t) sys_umask,
    [61]  = (syscall_t) sys_chroot,
    [63]  = (syscall_t) sys_dup2,
    [64]  = (syscall_t) sys_getppid,
    [65]  = (syscall_t) sys_getpgrp,
    [66]  = (syscall_t) sys_setsid,
    [74]  = (syscall_t) sys_sethostname,
    [75]  = (syscall_t) sys_setrlimit32,
    [76]  = (syscall_t) sys_old_getrlimit32,
    [77]  = (syscall_t) sys_getrusage,
    [78]  = (syscall_t) sys_gettimeofday,
    [79]  = (syscall_t) sys_settimeofday,
    [80]  = (syscall_t) sys_getgroups,
    [81]  = (syscall_t) sys_setgroups,
    [83]  = (syscall_t) sys_symlink,
    [85]  = (syscall_t) sys_readlink,
    [88]  = (syscall_t) sys_reboot,
    [90]  = (syscall_t) sys_mmap,
    [91]  = (syscall_t) sys_munmap,
    [94]  = (syscall_t) sys_fchmod,
    [96]  = (syscall_t) sys_getpriority,
    [97]  = (syscall_t) sys_setpriority,
    [99]  = (syscall_t) sys_statfs,
    [100] = (syscall_t) sys_fstatfs,
    [102] = (syscall_t) sys_socketcall,
    [103] = (syscall_t) sys_syslog,
    [104] = (syscall_t) sys_setitimer,
    [114] = (syscall_t) sys_wait4,
    [116] = (syscall_t) sys_sysinfo,
    [117] = (syscall_t) sys_ipc,
    [118] = (syscall_t) sys_fsync,
    [119] = (syscall_t) sys_sigreturn,
    [120] = (syscall_t) sys_clone,
    [122] = (syscall_t) sys_uname,
    [125] = (syscall_t) sys_mprotect,
    [132] = (syscall_t) sys_getpgid,
    [133] = (syscall_t) sys_fchdir,
    [136] = (syscall_t) sys_personality,
    [140] = (syscall_t) sys__llseek,
    [141] = (syscall_t) sys_getdents,
    [142] = (syscall_t) sys_select,
    [143] = (syscall_t) sys_flock,
    [144] = (syscall_t) sys_msync,
    [145] = (syscall_t) sys_readv,
    [146] = (syscall_t) sys_writev,
    [147] = (syscall_t) sys_getsid,
    [148] = (syscall_t) sys_fsync, // fdatasync
    [150] = (syscall_t) sys_mlock,
    [155] = (syscall_t) sys_sched_getparam,
    [156] = (syscall_t) sys_sched_setscheduler,
    [157] = (syscall_t) sys_sched_getscheduler,
    [158] = (syscall_t) sys_sched_yield,
    [159] = (syscall_t) sys_sched_get_priority_max,
    [162] = (syscall_t) sys_nanosleep,
    [163] = (syscall_t) sys_mremap,
    [168] = (syscall_t) sys_poll,
    [172] = (syscall_t) sys_prctl,
    [173] = (syscall_t) sys_rt_sigreturn,
    [174] = (syscall_t) sys_rt_sigaction,
    [175] = (syscall_t) sys_rt_sigprocmask,
    [176] = (syscall_t) sys_rt_sigpending,
    [177] = (syscall_t) sys_rt_sigtimedwait,
    [179] = (syscall_t) sys_rt_sigsuspend,
    [180] = (syscall_t) sys_pread,
    [181] = (syscall_t) sys_pwrite,
    [183] = (syscall_t) sys_getcwd,
    [184] = (syscall_t) sys_capget,
    [185] = (syscall_t) sys_capset,
    [186] = (syscall_t) sys_sigaltstack,
    [187] = (syscall_t) sys_sendfile,
    [190] = (syscall_t) sys_vfork,
    [191] = (syscall_t) sys_getrlimit32,
    [192] = (syscall_t) sys_mmap2,
    [193] = (syscall_t) sys_truncate64,
    [194] = (syscall_t) sys_ftruncate64,
    [195] = (syscall_t) sys_stat64,
    [196] = (syscall_t) sys_lstat64,
    [197] = (syscall_t) sys_fstat64,
    [198] = (syscall_t) sys_lchown,
    [199] = (syscall_t) sys_getuid32,
    [200] = (syscall_t) sys_getgid32,
    [201] = (syscall_t) sys_geteuid32,
    [202] = (syscall_t) sys_getegid32,
    [203] = (syscall_t) sys_setreuid,
    [204] = (syscall_t) sys_setregid,
    [205] = (syscall_t) sys_getgroups,
    [206] = (syscall_t) sys_setgroups,
    [207] = (syscall_t) sys_fchown32,
    [208] = (syscall_t) sys_setresuid,
    [209] = (syscall_t) sys_getresuid,
    [210] = (syscall_t) sys_setresgid,
    [211] = (syscall_t) sys_getresgid,
    [212] = (syscall_t) sys_chown32,
    [213] = (syscall_t) sys_setuid,
    [214] = (syscall_t) sys_setgid,
    [215] = (syscall_t) syscall_stub, // setfsuid
    [216] = (syscall_t) syscall_stub, // setfsgid
    [219] = (syscall_t) sys_madvise,
    [220] = (syscall_t) sys_getdents64,
    [221] = (syscall_t) sys_fcntl,
    [224] = (syscall_t) sys_gettid,
    [225] = (syscall_t) syscall_success_stub, // readahead
    [226 ... 237] = (syscall_t) sys_xattr_stub,
    [238] = (syscall_t) sys_tkill,
    [239] = (syscall_t) sys_sendfile64,
    [240] = (syscall_t) sys_futex,
    [241] = (syscall_t) sys_sched_setaffinity,
    [242] = (syscall_t) sys_sched_getaffinity,
    [243] = (syscall_t) sys_set_thread_area,
    [245] = (syscall_t) syscall_stub, // io_setup
    [252] = (syscall_t) sys_exit_group,
    [254] = (syscall_t) sys_epoll_create0,
    [255] = (syscall_t) sys_epoll_ctl,
    [256] = (syscall_t) sys_epoll_wait,
    [258] = (syscall_t) sys_set_tid_address,
    [259] = (syscall_t) sys_timer_create,
    [260] = (syscall_t) sys_timer_settime,
    [263] = (syscall_t) sys_timer_delete,
    [264] = (syscall_t) sys_clock_settime,
    [265] = (syscall_t) sys_clock_gettime,
    [266] = (syscall_t) sys_clock_getres,
    [268] = (syscall_t) sys_statfs64,
    [269] = (syscall_t) sys_fstatfs64,
    [270] = (syscall_t) sys_tgkill,
    [271] = (syscall_t) sys_utimes,
    [272] = (syscall_t) syscall_success_stub,
    [274] = (syscall_t) sys_mbind,
    [284] = (syscall_t) sys_waitid,
    [289] = (syscall_t) sys_ioprio_set,
    [290] = (syscall_t) sys_ioprio_get,
    [291] = (syscall_t) syscall_stub, // inotify_init
    [295] = (syscall_t) sys_openat,
    [296] = (syscall_t) sys_mkdirat,
    [297] = (syscall_t) sys_mknodat,
    [298] = (syscall_t) sys_fchownat,
    [300] = (syscall_t) sys_fstatat64,
    [301] = (syscall_t) sys_unlinkat,
    [302] = (syscall_t) sys_renameat,
    [303] = (syscall_t) sys_linkat,
    [304] = (syscall_t) sys_symlinkat,
    [305] = (syscall_t) sys_readlinkat,
    [306] = (syscall_t) sys_fchmodat,
    [307] = (syscall_t) sys_faccessat,
    [308] = (syscall_t) sys_pselect,
    [309] = (syscall_t) sys_ppoll,
    [311] = (syscall_t) sys_set_robust_list,
    [312] = (syscall_t) sys_get_robust_list,
    [313] = (syscall_t) sys_splice,
    [319] = (syscall_t) sys_epoll_pwait,
    [320] = (syscall_t) sys_utimensat,
    [322] = (syscall_t) sys_timerfd_create,
    [323] = (syscall_t) sys_eventfd,
    [324] = (syscall_t) sys_fallocate,
    [325] = (syscall_t) sys_timerfd_settime,
    [328] = (syscall_t) sys_eventfd2,
    [329] = (syscall_t) sys_epoll_create,
    [330] = (syscall_t) sys_dup3,
    [331] = (syscall_t) sys_pipe2,
    [332] = (syscall_t) syscall_stub, // inotify_init1
    [340] = (syscall_t) sys_prlimit64,
    [345] = (syscall_t) sys_sendmmsg,
    [352] = (syscall_t) syscall_stub, // sched_getattr
    [353] = (syscall_t) sys_renameat2,
    [355] = (syscall_t) sys_getrandom,
    [359] = (syscall_t) sys_socket,
    [360] = (syscall_t) sys_socketpair,
    [361] = (syscall_t) sys_bind,
    [362] = (syscall_t) sys_connect,
    [363] = (syscall_t) sys_listen,
    [364] = (syscall_t) syscall_stub, // accept4
    [365] = (syscall_t) sys_getsockopt,
    [366] = (syscall_t) sys_setsockopt,
    [367] = (syscall_t) sys_getsockname,
    [368] = (syscall_t) sys_getpeername,
    [369] = (syscall_t) sys_sendto,
    [370] = (syscall_t) sys_sendmsg,
    [371] = (syscall_t) sys_recvfrom,
    [372] = (syscall_t) sys_recvmsg,
    [373] = (syscall_t) sys_shutdown,
    [375] = (syscall_t) syscall_silent_stub, // membarrier
    [377] = (syscall_t) sys_copy_file_range,
    [383] = (syscall_t) sys_statx,
    [384] = (syscall_t) sys_arch_prctl,
    [422] = (syscall_t) syscall_silent_stub, // futex_time64
    [439] = (syscall_t) syscall_silent_stub, // faccessat2
};

#define NUM_SYSCALLS (sizeof(syscall_table) / sizeof(syscall_table[0]))

// x86-64 syscall table
// x86-64 uses different syscall numbers than x86-32
// Reference: https://github.com/torvalds/linux/blob/master/arch/x86/entry/syscalls/syscall_64.tbl
syscall_t syscall_table_64[] = {
    [0]   = (syscall_t) sys_read,              // read
    [1]   = (syscall_t) sys_write,             // write
    [2]   = (syscall_t) sys_open,              // open
    [3]   = (syscall_t) sys_close,             // close
    [4]   = (syscall_t) sys_stat64,            // stat
    [5]   = (syscall_t) sys_fstat64,           // fstat
    [6]   = (syscall_t) sys_lstat64,           // lstat
    [7]   = (syscall_t) sys_poll,              // poll
    [8]   = (syscall_t) sys_lseek,             // lseek
    [9]   = (syscall_t) sys_mmap2,             // mmap
    [10]  = (syscall_t) sys_mprotect,          // mprotect
    [11]  = (syscall_t) sys_munmap,            // munmap
    [12]  = (syscall_t) sys_brk,               // brk
    [13]  = (syscall_t) sys_rt_sigaction,      // rt_sigaction
    [14]  = (syscall_t) sys_rt_sigprocmask,    // rt_sigprocmask
    [15]  = (syscall_t) sys_rt_sigreturn,      // rt_sigreturn
    [16]  = (syscall_t) sys_ioctl,             // ioctl
    [17]  = (syscall_t) sys_pread,             // pread64
    [18]  = (syscall_t) sys_pwrite,            // pwrite64
    [19]  = (syscall_t) sys_readv,             // readv
    [20]  = (syscall_t) sys_writev,            // writev
    [21]  = (syscall_t) sys_access,            // access
    [22]  = (syscall_t) sys_pipe,              // pipe
    [23]  = (syscall_t) sys_select,            // select
    [24]  = (syscall_t) sys_sched_yield,       // sched_yield
    [25]  = (syscall_t) sys_mremap,            // mremap
    [26]  = (syscall_t) sys_msync,             // msync
    [32]  = (syscall_t) sys_dup,               // dup
    [33]  = (syscall_t) sys_dup2,              // dup2
    [34]  = (syscall_t) sys_pause,             // pause
    [35]  = (syscall_t) sys_nanosleep,         // nanosleep
    [37]  = (syscall_t) sys_alarm,             // alarm
    [39]  = (syscall_t) sys_getpid,            // getpid
    [41]  = (syscall_t) syscall_stub,          // socket
    [42]  = (syscall_t) syscall_stub,          // connect
    [43]  = (syscall_t) syscall_stub,          // accept
    [56]  = (syscall_t) sys_clone,             // clone
    [57]  = (syscall_t) sys_fork,              // fork
    [58]  = (syscall_t) sys_vfork,             // vfork
    [59]  = (syscall_t) sys_execve,            // execve
    [60]  = (syscall_t) sys_exit,              // exit
    [61]  = (syscall_t) sys_wait4,             // wait4
    [62]  = (syscall_t) sys_kill,              // kill
    [63]  = (syscall_t) sys_uname,             // uname
    [72]  = (syscall_t) sys_fcntl,             // fcntl
    [73]  = (syscall_t) sys_flock,             // flock
    [74]  = (syscall_t) sys_fsync,             // fsync
    [79]  = (syscall_t) sys_getcwd,            // getcwd
    [80]  = (syscall_t) sys_chdir,             // chdir
    [81]  = (syscall_t) sys_fchdir,            // fchdir
    [82]  = (syscall_t) sys_rename,            // rename
    [83]  = (syscall_t) sys_mkdir,             // mkdir
    [84]  = (syscall_t) sys_rmdir,             // rmdir
    [85]  = (syscall_t) sys_link,              // link
    [86]  = (syscall_t) sys_unlink,            // unlink
    [87]  = (syscall_t) sys_symlink,           // symlink
    [88]  = (syscall_t) sys_readlink,          // readlink
    [89]  = (syscall_t) sys_chmod,             // chmod
    [90]  = (syscall_t) sys_fchmod,            // fchmod
    [91]  = (syscall_t) sys_chown32,           // chown
    [92]  = (syscall_t) sys_fchown32,          // fchown
    [93]  = (syscall_t) sys_lchown,            // lchown
    [94]  = (syscall_t) sys_umask,             // umask
    [95]  = (syscall_t) sys_gettimeofday,      // gettimeofday
    [96]  = (syscall_t) sys_getrlimit32,       // getrlimit
    [97]  = (syscall_t) sys_getrusage,         // getrusage
    [98]  = (syscall_t) sys_sysinfo,           // sysinfo
    [99]  = (syscall_t) sys_times,             // times
    [102] = (syscall_t) sys_getuid32,          // getuid
    [103] = (syscall_t) sys_syslog,            // syslog
    [104] = (syscall_t) sys_getgid32,          // getgid
    [105] = (syscall_t) sys_setuid,            // setuid
    [106] = (syscall_t) sys_setgid,            // setgid
    [107] = (syscall_t) sys_geteuid32,         // geteuid
    [108] = (syscall_t) sys_getegid32,         // getegid
    [109] = (syscall_t) sys_setpgid,           // setpgid
    [110] = (syscall_t) sys_getppid,           // getppid
    [111] = (syscall_t) sys_getpgrp,           // getpgrp
    [112] = (syscall_t) sys_setsid,            // setsid
    [113] = (syscall_t) sys_setreuid,          // setreuid
    [114] = (syscall_t) sys_setregid,          // setregid
    [115] = (syscall_t) sys_getgroups,         // getgroups
    [116] = (syscall_t) sys_setgroups,         // setgroups
    [117] = (syscall_t) sys_setresuid,         // setresuid
    [118] = (syscall_t) sys_getresuid,         // getresuid
    [119] = (syscall_t) sys_setresgid,         // setresgid
    [120] = (syscall_t) sys_getresgid,         // getresgid
    [121] = (syscall_t) sys_getpgid,           // getpgid
    [124] = (syscall_t) sys_getsid,            // getsid
    [125] = (syscall_t) sys_capget,            // capget
    [126] = (syscall_t) sys_capset,            // capset
    [131] = (syscall_t) sys_sigaltstack,       // sigaltstack
    [137] = (syscall_t) sys_statfs,            // statfs
    [138] = (syscall_t) sys_fstatfs,           // fstatfs
    [157] = (syscall_t) sys_prctl,             // prctl
    [158] = (syscall_t) sys_arch_prctl,        // arch_prctl
    [186] = (syscall_t) sys_gettid,            // gettid
    [202] = (syscall_t) sys_futex,             // futex
    [217] = (syscall_t) sys_getdents64,        // getdents64
    [218] = (syscall_t) sys_set_tid_address,   // set_tid_address
    [228] = (syscall_t) sys_clock_gettime,     // clock_gettime
    [229] = (syscall_t) sys_clock_getres,      // clock_getres
    [230] = (syscall_t) sys_clock_settime,     // clock_settime
    [231] = (syscall_t) sys_exit_group,        // exit_group
    [232] = (syscall_t) sys_epoll_wait,        // epoll_wait
    [233] = (syscall_t) sys_epoll_ctl,         // epoll_ctl
    [234] = (syscall_t) sys_tkill,             // tgkill
    [257] = (syscall_t) sys_openat,            // openat
    [258] = (syscall_t) sys_mkdirat,           // mkdirat
    [259] = (syscall_t) sys_mknodat,           // mknodat
    [260] = (syscall_t) sys_fchownat,          // fchownat
    [261] = (syscall_t) sys_fstatat64,         // newfstatat
    [262] = (syscall_t) sys_unlinkat,          // unlinkat
    [263] = (syscall_t) sys_renameat,          // renameat
    [264] = (syscall_t) sys_linkat,            // linkat
    [265] = (syscall_t) sys_symlinkat,         // symlinkat
    [266] = (syscall_t) sys_readlinkat,        // readlinkat
    [267] = (syscall_t) sys_fchmodat,          // fchmodat
    [268] = (syscall_t) sys_faccessat,         // faccessat
    [269] = (syscall_t) sys_pselect,           // pselect6
    [270] = (syscall_t) sys_ppoll,             // ppoll
    [271] = (syscall_t) syscall_stub,          // unshare
    [272] = (syscall_t) sys_set_thread_area,   // set_robust_list (mapped to set_thread_area for now)
    [281] = (syscall_t) sys_epoll_create0,     // epoll_create1
    [282] = (syscall_t) sys_dup3,              // dup3
    [283] = (syscall_t) sys_pipe2,             // pipe2
    [285] = (syscall_t) sys_eventfd2,          // eventfd2
    [286] = (syscall_t) sys_epoll_pwait,       // epoll_pwait
    [291] = (syscall_t) sys_epoll_create0,     // epoll_create1 (dup)
    [319] = (syscall_t) sys_getrandom,         // getrandom
    [332] = (syscall_t) sys_statx,             // statx
};

#define NUM_SYSCALLS_64 (sizeof(syscall_table_64) / sizeof(syscall_table_64[0]))

void dump_stack(int lines);

void handle_interrupt(int interrupt) {
    struct cpu_state *cpu = &current->cpu;
    if (interrupt == INT_SYSCALL) {
        unsigned syscall_num = cpu->eax;
        if (syscall_num >= NUM_SYSCALLS || syscall_table[syscall_num] == NULL) {
            printk("%d(%s) missing syscall %d\n", current->pid, current->comm, syscall_num);
            cpu->eax = _ENOSYS;
        } else {
            if (syscall_table[syscall_num] == (syscall_t) syscall_stub) {
                printk("%d(%s) stub syscall %d\n", current->pid, current->comm, syscall_num);
            }
            STRACE("%d call %-3d ", current->pid, syscall_num);
            int result = syscall_table[syscall_num](cpu->ebx, cpu->ecx, cpu->edx, cpu->esi, cpu->edi, cpu->ebp);
            STRACE(" = 0x%x\n", result);
            cpu->eax = result;
        }
    } else if (interrupt == INT_SYSCALL64) {
        // 64-bit syscall handler
        // x86-64 calling convention: rax=syscall#, rdi,rsi,rdx,r10,r8,r9=args, rax=return
        unsigned syscall_num = (unsigned) cpu->regs[0]; // RAX
        if (syscall_num >= NUM_SYSCALLS_64 || syscall_table_64[syscall_num] == NULL) {
            printk("%d(%s) missing 64-bit syscall %d\n", current->pid, current->comm, syscall_num);
            cpu->regs[0] = _ENOSYS; // Return in RAX
        } else {
            if (syscall_table_64[syscall_num] == (syscall_t) syscall_stub) {
                printk("%d(%s) stub 64-bit syscall %d\n", current->pid, current->comm, syscall_num);
            }
            STRACE("%d call64 %-3d ", current->pid, syscall_num);
            // Arguments: RDI, RSI, RDX, R10, R8, R9
            int result = syscall_table_64[syscall_num](
                (dword_t) cpu->regs[7],  // RDI
                (dword_t) cpu->regs[6],  // RSI
                (dword_t) cpu->regs[2],  // RDX
                (dword_t) cpu->regs[10], // R10
                (dword_t) cpu->regs[8],  // R8
                (dword_t) cpu->regs[9]   // R9
            );
            STRACE(" = 0x%x\n", result);
            cpu->regs[0] = result; // Return in RAX
        }
    } else if (interrupt == INT_GPF) {
        // some page faults, such as stack growing or CoW clones, are handled by mem_ptr
        read_wrlock(&current->mem->lock);
        void *ptr = mem_ptr(current->mem, cpu->segfault_addr, cpu->segfault_was_write ? MEM_WRITE : MEM_READ);
        read_wrunlock(&current->mem->lock);
        if (ptr == NULL) {
            printk("%d page fault on 0x%llx at 0x%x\n", current->pid, (unsigned long long)cpu->segfault_addr, cpu->eip);
            struct siginfo_ info = {
                .code = mem_segv_reason(current->mem, cpu->segfault_addr),
                .fault.addr = cpu->segfault_addr,
            };
            dump_stack(8);
            deliver_signal(current, SIGSEGV_, info);
        }
    } else if (interrupt == INT_UNDEFINED) {
        printk("%d illegal instruction at 0x%x: ", current->pid, cpu->eip);
        for (int i = 0; i < 8; i++) {
            uint8_t b;
            if (user_get(cpu->eip + i, b))
                break;
            printk("%02x ", b);
        }
        printk("\n");
        dump_stack(8);
        struct siginfo_ info = {
            .code = SI_KERNEL_,
            .fault.addr = cpu->eip,
        };
        deliver_signal(current, SIGILL_, info);
    } else if (interrupt == INT_BREAKPOINT) {
        lock(&pids_lock);
        send_signal(current, SIGTRAP_, (struct siginfo_) {
            .sig = SIGTRAP_,
            .code = SI_KERNEL_,
        });
        unlock(&pids_lock);
    } else if (interrupt == INT_DEBUG) {
        lock(&pids_lock);
        send_signal(current, SIGTRAP_, (struct siginfo_) {
            .sig = SIGTRAP_,
            .code = TRAP_TRACE_,
        });
        unlock(&pids_lock);
    } else if (interrupt != INT_TIMER) {
        printk("%d unhandled interrupt %d\n", current->pid, interrupt);
        sys_exit(interrupt);
    }

    receive_signals();
    struct tgroup *group = current->group;
    lock(&group->lock);
    while (group->stopped)
        wait_for_ignore_signals(&group->stopped_cond, &group->lock, NULL);
    unlock(&group->lock);
}

void dump_maps(void) {
    extern void proc_maps_dump(struct task *task, struct proc_data *buf);
    struct proc_data buf = {};
    proc_maps_dump(current, &buf);
    // go a line at a time because it can be fucking enormous
    char *orig_data = buf.data;
    while (buf.size > 0) {
        size_t chunk_size = buf.size;
        if (chunk_size > 1024)
            chunk_size = 1024;
        printk("%.*s", chunk_size, buf.data);
        buf.data += chunk_size;
        buf.size -= chunk_size;
    }
    free(orig_data);
}

void dump_mem(addr_t start, uint_t len) {
    const int width = 8;
    for (addr_t addr = start; addr < start + len; addr += sizeof(dword_t)) {
        unsigned from_left = (addr - start) / sizeof(dword_t) % width;
        if (from_left == 0)
            printk("%08x: ", addr);
        dword_t word;
        if (user_get(addr, word))
            break;
        printk("%08x ", word);
        if (from_left == width - 1)
            printk("\n");
    }
}

void dump_stack(int lines) {
    printk("stack at %x, base at %x, ip at %x\n", current->cpu.esp, current->cpu.ebp, current->cpu.eip);
    dump_mem(current->cpu.esp, lines * sizeof(dword_t) * 8);
}

// TODO find a home for this
#ifdef LOG_OVERRIDE
int log_override = 0;
#endif
