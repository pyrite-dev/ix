.PHONY: all depend clean

MACH = micro960
ARCH = i960
CONFIG = GENERIC
TOP = .

include sys/mach/$(MACH)/config/$(CONFIG)

all: compile

compile: depend
	cd $@ && $(MAKE) MACH=$(MACH) ARCH=$(ARCH) CONFIG=$(CONFIG) TOP=..

depend:
	mkdir -p compile
	./tools/prelude.sh kernel $(KERNEL) > compile/Makefile
	./tools/makedep.sh `find sys/mach/$(MACH) sys/arch/$(ARCH) sys/common -name "*.c" -or -name "*.s" -or -name "*.S"` >> compile/Makefile
	cat sys/mach/$(MACH)/Makefile.in >> compile/Makefile

clean:
	rm -rf compile
