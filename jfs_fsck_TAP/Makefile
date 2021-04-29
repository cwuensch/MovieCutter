APP = jfs_fsck_TAP
BASE = $(shell cd /tapapi/TMS; pwd)
CUR_DIR = $(shell pwd)
JFS_OBJS = jfs_fsck/xchkdsk.o jfs_fsck/fsckwsp.o jfs_fsck/fsck_message.o jfs_fsck/fsckmsgdef.o jfs_fsck/fsckpfs.o jfs_fsck/fsckino.o jfs_fsck/fsckimap.o jfs_fsck/unicode_to_utf8.o jfs_fsck/fsckdtre.o jfs_fsck/fsckmeta.o jfs_fsck/diskmap.o jfs_fsck/super.o jfs_fsck/logredo.o jfs_fsck/fsckbmap.o jfs_fsck/logform.o jfs_fsck/utilsubs.o jfs_fsck/dirindex.o jfs_fsck/fssubs.o jfs_fsck/devices.o jfs_fsck/fsckxtre.o jfs_fsck/fsckea.o jfs_fsck/fsckdire.o jfs_fsck/uniupr.o jfs_fsck/log_map.o jfs_fsck/open_by_label.o jfs_fsck/log_read.o jfs_fsck/log_work.o jfs_fsck/inode.o jfs_fsck/lib.o jfs_fsck/jfs_icheck.o

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
CFLAGS += -DLINUX  -MD -W -Wall -O2 -fno-strict-aliasing
CPPFLAGS += -DLINUX  -MD -W -Wall  -O2

INCLUDE_DIRS = ${BASE}/include /opt/crosstool/include/
TAP_INCLUDE_DIRS = $(addprefix -I, $(INCLUDE_DIRS))
TAP_FLAGS += $(TAP_INCLUDE_DIRS)
TAP_LIBS = ${BASE}/tapinit.o ${BASE}/libtap.so -lgcc_s -lFireBird -ldl -luuid -lc

all: $(APP).tap clean

$(APP).tap: $(APP).o $(JFS_OBJS)
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
	@-${RM} jfs_fsck/*.d
	