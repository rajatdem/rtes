cmd_rtes/kernel/built-in.o :=  arm-linux-gnueabi-ld -EL    -r -o rtes/kernel/built-in.o rtes/kernel/ps.o rtes/kernel/calc.o rtes/kernel/reserve.o rtes/kernel/end_job.o rtes/kernel/taskmon_sysfs.o rtes/kernel/cpu_heuristics.o rtes/kernel/sysclock.o 
