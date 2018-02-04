#ifndef _RESERVE_H
#define _RESERVE_H

#define __NR_SYSCALL_RESERVE_SET 379
#define __NR_SYSCALL_RESERVE_CANCEL 380

#define set_reserve(a, b, c, d) syscall(__NR_SYSCALL_RESERVE_SET, a, b, c, d)
#define cancel_reserve(a) syscall(__NR_SYSCALL_RESERVE_CANCEL, a)

#define ONE_SEC 1000
#define ONE_SEC_NS 1000000

#define SIGEXCESS 33
#define true 1
#define false 0

#endif
