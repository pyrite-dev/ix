#!/bin/sh
while read i; do
	INPUT="../$i"
	OUTPUT="./$1/`echo "$i" | sed 's/\//_/g' | cut -d. -f1`.o"

	echo "OBJS += $OUTPUT"
	echo "$OUTPUT: $INPUT"
	case "$INPUT" in
	*.c)
		echo "	\$(CC) \$(CPPFLAGS) \$(CFLAGS) -c -o \$@ \$<"
		;;
	*.s)
		echo "	\$(AS) \$(ASFLAGS) -o \$@ \$<"
		;;
	*.S)
		echo "	\$(CPP) \$(CPPFLAGS) \$< | \$(AS) \$(ASFLAGS) -o \$@"
		;;
	esac
	echo ""
done
