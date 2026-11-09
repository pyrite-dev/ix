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
	mkdir -p compile/lib/libc
	./tools/config.pl sys/mach/$(MACH)/conf/$(CONFIG)
	$(INHERIT) ./tools/mkmake.sh all > compile/Makefile
	$(INHERIT) ./tools/mkmake.sh kernel > compile/Makefile.kernel
	$(INHERIT) ./tools/mkmake.sh libc > compile/Makefile.libc

format:
	clang-format --verbose -i `find sys lib include -name "*.c" -or -name "*.h"`

clean:
	rm -rf compile
