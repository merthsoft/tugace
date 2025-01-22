# ----------------------------
# Makefile Options
# ----------------------------

NAME = TURTLE
ICON = icon.png
DESCRIPTION = "CE Turtles"
COMPRESSED = YES
ARCHIVED = NO

CFLAGS = -Wall -Wextra -Oz
CXXFLAGS = -Wall -Wextra -Oz

PREFER_OS_LIBC=NO

DEBUGMODE = DEBUG

# ----------------------------

include $(shell cedev-config --makefile)
