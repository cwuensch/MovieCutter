APP = jfsRepair
BASE = $(shell cd /tapapi/TMS; pwd)
CUR_DIR = $(shell pwd)
JFS_OBJS = jfs_objects/inode.o jfs_objects/ui.o jfs_objects/iag.o jfs_objects/io.o jfs_objects/xpeek.o jfs_objects/alter.o jfs_objects/directory.o jfs_objects/dmap.o jfs_objects/display.o jfs_objects/fsckcbbl.o jfs_objects/help.o jfs_objects/super.o jfs_objects/super2.o jfs_objects/libfs.a 

# include default settings
include ${BASE}/include/tool.mk

# output object directory
BIN_DIR = ${CUR_DIR}/bin
SRC_DIR = ${CUR_DIR}

TAP_FLAGS += -fPIC

ifeq (${DEBUG},y)
TAP_FLAGS += -g -DDEBUG
endif

#compile options
CFLAGS += -DLINUX  -MD -W -Wall -O2 -mips32
CPPFLAGS += -DLINUX  -MD -W -Wall  -O2 -mips32

INCLUDE_DIRS = ${BASE}/include /opt/crosstool/include/
TAP_INCLUDE_DIRS = $(addprefix -I, $(INCLUDE_DIRS))
TAP_FLAGS += $(TAP_INCLUDE_DIRS)
TAP_LIBS = ${BASE}/tapinit.o ${BASE}/libtap.so -lFireBird -ldl -luuid -lc

all: $(APP).tap clean

$(APP).tap: $(APP).o
	@echo "[Linking... $@]"
	@$(LD) -shared --no-undefined --allow-shlib-undefined $(JFS_OBJS) -o $@ $< $(TAP_LIBS)
	@$(JB) $@
	@-${RM} $(subst .tap,_TMS.tap,$@)

%.o: %.c ${BASE}/include/libFireBird.h
	@echo "[Compiling... $@]"
	@$(CC) $(CFLAGS) $(TAP_FLAGS) -c $< -o $@

clean:
	@echo "[Cleaning up...]"
	@-${RM} *.d
