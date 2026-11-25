#!/bin/sh
echo "include \$(TOP)/compile/Makefile.config"
echo ""
if [ -f "$1/Makefile.prelude" ]; then
	cat $1/Makefile.prelude
fi
echo 'CPPFLAGS += -I$(TOP)/sys/include -I$(TOP)/sys/mach/$(MACH)/include -I$(TOP)/sys/arch/$(ARCH)/include -I$(TOP)/include'
echo ".PHONY: all clean"
echo ""
echo "all: $2"
echo ""
echo "clean:"
echo "	rm -f $2 \$(OBJS)"
echo ""
