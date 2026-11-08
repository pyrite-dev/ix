#!/bin/sh
if [ "$1" = "all" ]; then
	echo ".PHONY: all kernel clean"
	echo ""
	echo "all: kernel"
	echo ""
	echo "kernel:"
	echo "	\$(MAKE) -f Makefile.kernel"
	echo ""
	echo "clean:"
	echo "	\$(MAKE) -f Makefile.kernel clean"
elif [ "$1" = "kernel" ]; then
	./tools/prelude.sh sys "\$(KERNEL)"
	(
		MATCH='-name "*.c" -or -name "*.s" -or -name "*.S"'

		eval find \
			sys/mach/$MACH/$MACH \
			sys/arch/$ARCH \
			sys/common \
			-maxdepth 1 $MATCH
		ls -d lib/csu/arch/$ARCH/crt0.*
	) | ./tools/makedep.sh sys 2>/dev/null
	cat sys/mach/$MACH/Makefile.in
	cat sys/Makefile.in
fi
