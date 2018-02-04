#ifndef __CALC_H_
#define __CALC_H_

#define __NR_SYSCALL_CALC 378

#define calc(a, b, c, d) syscall(__NR_SYSCALL_CALC, a, b, c, d)

#endif
