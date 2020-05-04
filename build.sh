#!/bin/bash

PROGNAME="countdown"

SOURCEDIR="./src"
BUILDDIR="./build"

FILES=(
    "$SOURCEDIR/main.cpp"
)

TESTBUILD=

TESTSOURCEDIR="./test"
TESTBUILDDIR="./build"

TESTFILES=(
    "$TESTSOURCEDIR/maintest.cpp"
)

DEBUGFLAGS="-g -ggdb -DDEBUG_BUILD=1"
DEBUGEXT="_debug"
if [ "$1" = "-R" ]; then
    DEBUGFLAGS="-DDEBUG_BUILD=0"
    DEBUGEXT=""
    shift
fi

FULLNAME="${PROGNAME}${DEBUGEXT}"
DEFINES="-DPROGRAM_NAME=\"${FULLNAME}\""

CXXFLAGS="${DEBUGFLAGS} -O3 -pedantic -pipe -I./include $(pkg-config --cflags x11 xrandr)"
WARNINGS="-Wall -Wextra -Wno-unused-variable -Wno-unused-parameter -Wno-unused-function -Wno-writable-strings"
LDLIBS="$(pkg-config --libs x11 xrandr)"

[ -d "$BUILDDIR" ] || mkdir $BUILDDIR
time clang++ ${CXXFLAGS} ${WARNINGS} ${DEFINES} "${@}" -o ${BUILDDIR}/${FULLNAME} ${FILES[*]} ${LDLIBS}
RESULT=$?

[ "$TESTBUILD" ] || exit $RESULT

TESTCXXFLAGS="$CXXFLAGS $(pkg-config --cflags gtest)"
TESTWARNINGS="$WARNINGS"
TESTLDLIBS="$LDLIBS $(pkg-config --libs gtest)"

time clang++ ${TESTCXXFLAGS} ${TESTWARNINGS} ${DEFINES} "${@}" -o ${TESTBUILDDIR}/${FULLNAME}_test ${TESTFILES[*]} ${TESTLDLIBS}
