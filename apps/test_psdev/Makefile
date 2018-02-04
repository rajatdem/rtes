CC=arm-linux-androideabi-gcc 
SYSROOT=$(HOME)/android-ndk-r9/platforms/android-14/arch-arm

NAME=test
FILES=test.c
RES=res
RES_C=res_test.c
res:
	$(CC) --sysroot $(SYSROOT) $(RES_C) -o $(RES)
all:
	$(CC) --sysroot $(SYSROOT) $(FILES) -o $(NAME)
