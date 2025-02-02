# ----------------------------
# Makefile Options
# ----------------------------

NAME = TUGA
ICON = icon.png
DESCRIPTION = "TUGA Interpreter"
COMPRESSED = YES
ARCHIVED = YES

CFLAGS = -Wall -Wextra -Oz
CXXFLAGS = -Wall -Wextra -Oz

PREFER_OS_CRT = NO
PREFER_OS_LIBC = YES

DEBUGMODE = DEBUG

# ----------------------------

include $(shell cedev-config --makefile)
