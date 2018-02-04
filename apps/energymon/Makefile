CC=arm-linux-androideabi-gcc
SYSROOT=$(HOME)/android-ndk-r9/platforms/android-14/arch-arm

NAME=energymon
FILES=energymon.c

all:
	    $(CC) --sysroot $(SYSROOT) $(FILES) -o $(NAME)
clean:
	        rm $(NAME)

