#!/bin/sh
echo "include Makefile.config"
echo ""
if [ -f "$1/Makefile.prelude" ]; then
	cat $1/Makefile.prelude
fi
echo ".PHONY: all clean"
echo ""
echo "all: $2"
echo ""
