#!/bin/sh

ARCH=$(echo "$2" | tr [a-z] [A-Z])

UPPER_CASE_BUILD_TYPE=$(echo "$1" | tr [a-z] [A-Z])

LOWER_CASE_BUILD_TYPE=$(echo "$1" | tr [A-Z] [a-z])

VERSION=$(git describe --tags HEAD --always | cut -c2-)-${LOWER_CASE_BUILD_TYPE}

echo  "#ifndef AWOO_BUILD_INFO_H"
echo  "#define AWOO_BUILD_INFO_H"
echo  "#define AWOO_SCM_REV \"$(git show | head -n1 | cut -c8-)\""
echo  "#define AWOO_BUILD_TYPE \"${UPPER_CASE_BUILD_TYPE}\""
echo  "#define AWOO_VERSION \"${VERSION}\""
echo  "#define AWOO_ARCH \"${ARCH}\""
echo  "#define AWOO_${ARCH}"
echo  "#define AWOO_FULL_NAME \"awooOS ${ARCH} ${VERSION}\""
echo  "#endif"
