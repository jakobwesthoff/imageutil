GCC=/usr/bin/gcc
SH4-GCC=/usr/bin/sh4-unknown-linux-gnu-gcc

.PHONY: all
all: imageutil

kathi/nethelper: kathi/nethelper.c kathi/nethelper.h crc/crc.c crc/crc.h
	${SH4-GCC} -Os kathi/nethelper.c crc/crc.c -o kathi/nethelper

nethelper_resource.h: kathi/nethelper
	@echo Generating nethelper_resource.h
	@../utils/bin2c kathi/nethelper nethelper_resource NETHELPER_DATA

imageutil: imageutil.c imageutil.h nethelper_resource.h crc/crc.c crc/crc.h
	${GCC} imageutil.c crc/crc.c -o imageutil

.PHONY: clean
clean:
	-rm kathi/nethelper >/dev/null 2>&1
	-rm nethelper_resource.h >/dev/null 2>&1
	-rm imageutil >/dev/null 2>&1
	-rm imageutil-*.tar.gz >/dev/null 2>&1

.PHONY: package
package: imageutil
	@../utils/mkpackage.sh
