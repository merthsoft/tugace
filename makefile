# ----------------------------
# Makefile Options
# ----------------------------

NAME = TUGA
ICON = icon.png
DESCRIPTION = "TUGA Interpreter"
COMPRESSED = YES
ARCHIVED = YES
#-Rpass=inline -Rpass-missed=inline -v
CFLAGS = -Wall -Wextra -Ofast
CXXFLAGS = -Wall -Wextra -Oz

PREFER_OS_CRT = NO
PREFER_OS_LIBC = NO

DEBUGMODE = DEBUG

# ----------------------------

include $(shell cedev-config --makefile)