.PHONY: all depend format clean

MACH = micro960
ARCH = i960
CONFIG = GENERIC
TOP = .

INHERIT = MACH=$(MACH) ARCH=$(ARCH) CONFIG=$(CONFIG) TOP=..

all: world

world: compile
	cd compile && $(INHERIT) $(MAKE)

compile:
	$(INHERIT) $(MAKE) depend

depend:
	mkdir -p compile
	mkdir -p compile/sys
	./tools/config.pl sys/mach/$(MACH)/conf/$(CONFIG)
	$(INHERIT) ./tools/mkmake.sh all > compile/Makefile
	$(INHERIT) ./tools/mkmake.sh kernel > compile/Makefile.kernel

format:
	clang-format --verbose -i `find sys lib -name "*.c" -or -name "*.h"`

clean:
	rm -rf compile
