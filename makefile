# ----------------------------
# Makefile Options
# ----------------------------

NAME = TUGA
ICON = icon.png
DESCRIPTION = "TUGA Interpreter"
COMPRESSED = NO
ARCHIVED = YES
#-Rpass=inline -Rpass-missed=inline -v
CFLAGS = -Wall -Wextra -Ofast -Rpass-missed=inline
CXXFLAGS = -Wall -Wextra -Oz

PREFER_OS_CRT = NO
PREFER_OS_LIBC = NO
LTO = NO

DEBUGMODE = DEBUG

# ----------------------------

include $(shell cedev-config --makefile)