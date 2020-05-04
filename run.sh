#!/bin/bash

PROGNAME="countdown"

DEBUGEXT="_debug"
if [ "$1" = "-R" ]; then
    DEBUGEXT=
    shift
fi

FULLNAME="${PROGNAME}${DEBUGEXT}"

PREFIX=
if [ "$1" = "-d" ]; then PREFIX="lldb --"; shift
elif [ "$1" = "-v" ]; then PREFIX="valgrind --leak-check=full --"; shift
elif [ "$1" = "-vs" ]; then PREFIX="valgrind --leak-check=full --show-leak-kinds=all --"; shift
elif [ "$1" = "-vt" ]; then PREFIX="valgrind --leak-check=full --track-origins=yes --"; shift
elif [ "$1" = "-vst" ]; then PREFIX="valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --"; shift
fi

${PREFIX} ./build/${FULLNAME} "${@}"
