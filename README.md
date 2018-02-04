
# Real Time Embedded Systems (18-648) Project

Hacking the Linux Kernel for the Android Nexus 7 device

## /apps

## /modules: Loadable Kernel Modules
* hello: A simple module to print hello on the dmesg screen
* psdev: A character device driver supporting 255 devices
* cleanup: Module to check open devices

## /kernel
* sysclock.c: Computes the frequency to scale to based on utilization of the core.
* calc.c: Syscall to float arithmetic in kernel space.
* reserve.c: Reserves real-time threads. 
* cpu_heuristics.c: Computes core to be used to reserve the new thread based on user selected heuristic.
* taskmon_sysfs.c: Computes and stores energy consumption for each thread.

