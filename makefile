# ----------------------------
# Makefile Options
# ----------------------------

NAME = TURTINT
ICON = icon.png
DESCRIPTION = "CE Turtles"
COMPRESSED = YES
ARCHIVED = NO

CFLAGS = -Wall -Wextra -Ofast
CXXFLAGS = -Wall -Wextra -Oz

PREFER_OS_LIBC=NO

DEBUGMODE = DEBUG

# ----------------------------

include $(shell cedev-config --makefile)
