#!/bin/sh

echo "Creating package file."

TMP=`tempfile`
rm "${TMP}"
mkdir "${TMP}"

VERSION=`cat imageutil.h|grep "#define PROGRAM_VERSION"|sed -e 's@^#define PROGRAM_VERSION "v\. \([^"]\+\).*$@\1@'`
mkdir "${TMP}/imageutil-${VERSION}"
cp imageutil "${TMP}/imageutil-${VERSION}/"

CWD=`pwd`
cd "${TMP}"

tar cvzf "${CWD}/imageutil-${VERSION}.tar.gz" "imageutil-${VERSION}"

cd "${CWD}"

rm -rf "${TMP}"
