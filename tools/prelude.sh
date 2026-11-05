#!/bin/sh

echo "include \$(TOP)/sys/mach/\$(MACH)/config/\$(CONFIG)"
echo ""
if [ "$1" = "kernel" ]; then
	echo "CPPFLAGS += -D__KERNEL__"
	echo ""
fi
echo ".PHONY: all clean"
echo ""
echo "all: $2"
echo ""
