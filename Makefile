AS = i960-unknown-coff-as
CC = i960-unknown-coff-gcc
LD = i960-unknown-coff-ld
OBJCOPY = i960-unknown-coff-objcopy

OBJS = src/boot.o src/main.o

.PHONY: all clean
.SUFFIXES: .c .s .o

all: os.bin

os.bin: src/os.out
	$(OBJCOPY) -O binary --gap-fill 0xff src/os.out $@

src/os.out: src/linker.ld $(OBJS)
	$(LD) -ASA -T src/linker.ld -o $@ $(OBJS)

.s.o:
	$(AS) -ASA -o $@ $<

.c.o:
	$(CC) -msa -c -o $@ $<

clean:
	rm -f os.bin src/*.out src/*.o
