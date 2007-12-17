GCC=/usr/bin/gcc
SH4-GCC=/usr/bin/sh4-unknown-linux-gnu-gcc

.PHONY: all
all: imageutil

kathi/nethelper: kathi/nethelper.c kathi/nethelper.h
	${SH4-GCC} kathi/nethelper.c -o kathi/nethelper

nethelper_resource.h: kathi/nethelper
	@echo Generating nethelper_resource.h
	@../utils/bin2c kathi/nethelper nethelper_resource NETHELPER_DATA

imageutil: imageutil.c imageutil.h nethelper_resource.h
	${GCC} imageutil.c -o imageutil

.PHONY: clean
clean:
	-rm kathi/nethelper >/dev/null 2>&1
	-rm nethelper_resource.h >/dev/null 2>&1
	-rm imageutil >/dev/null 2>&1

