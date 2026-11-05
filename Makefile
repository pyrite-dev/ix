.PHONY: all clean

MACH = micro960
ARCH = i960
CONFIG = GENERIC
TOP = .

include sys/mach/$(MACH)/config/$(CONFIG)

INHERIT = MACH=$(MACH) ARCH=$(ARCH) CONFIG=$(CONFIG) KERNEL=$(KERNEL) TOP=..

all: world

world: compile
	cd compile && $(INHERIT) $(MAKE)

compile:
	mkdir -p compile
	mkdir -p compile/sys
	$(INHERIT) ./tools/mkmake.sh all > compile/Makefile
	$(INHERIT) ./tools/mkmake.sh kernel > compile/Makefile.kernel

clean:
	rm -rf compile
