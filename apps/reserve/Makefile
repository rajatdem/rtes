CC=arm-linux-androideabi-gcc 
SYSROOT=$(HOME)/android-ndk-r9/platforms/android-14/arch-arm

NAME=reserve
FILES=reserve.c
all:
	$(CC) --sysroot $(SYSROOT) $(FILES) -o $(NAME)
clean:
	rm $(NAME)
