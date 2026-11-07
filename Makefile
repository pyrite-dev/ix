.PHONY: all clean

MACH = micro960
ARCH = i960
CONFIG = GENERIC
TOP = .

INHERIT = MACH=$(MACH) ARCH=$(ARCH) CONFIG=$(CONFIG) TOP=..

all: world

world: compile
	cd compile && $(INHERIT) $(MAKE)

compile:
	mkdir -p compile
	mkdir -p compile/sys
	$(INHERIT) ./tools/mkmake.sh all > compile/Makefile
	$(INHERIT) ./tools/mkmake.sh kernel > compile/Makefile.kernel
	./tools/config.pl sys/mach/$(MACH)/config/$(CONFIG)

clean:
	rm -rf compile
