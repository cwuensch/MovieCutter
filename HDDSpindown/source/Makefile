APP1 = HDDSpindown
BASE = $(shell cd /tapapi/TMS; pwd)
CUR_DIR = $(shell pwd)

# include default settings
include ${BASE}/include/tool.mk

# output object directory
SRC_DIR = ${CUR_DIR}

TAP_FLAGS += -fPIC -fno-strict-aliasing

ifeq (${DEBUG},y)
TAP_FLAGS += -g -DDEBUG
endif

#compile options
CFLAGS += -DLINUX -D_REENTRANT  -MD  -W -Wall  -O2 -static -pipe -ffunction-sections -fdata-sections
CPPFLAGS += -DLINUX  -MD  -W -Wall  -O2 -static -pipe -ffunction-sections -fdata-sections

INCLUDE_DIRS = ${BASE}/include /opt/crosstool/include/
TAP_INCLUDE_DIRS = $(addprefix -I, $(INCLUDE_DIRS))
TAP_FLAGS += $(TAP_INCLUDE_DIRS)
TAP_LIBS = ${BASE}/tapinit.o ${BASE}/libtap.so -lFireBird -ldl -lc

all: $(APP1).tap clean

$(APP1).tap: $(APP1).o sgio.o sysfs.o apt.o
	@echo "[Linking... $@]"
	@$(LD) -shared --no-undefined --allow-shlib-undefined -O1 --gc-sections sgio.o sysfs.o apt.o -o $@ $< $(TAP_LIBS)
	@$(JB) $@

%.o: %.c ${BASE}/include/libFireBird.h
	@echo "[Compiling... $@]"
	@$(CC) $(CFLAGS) $(TAP_FLAGS) -c $< -o $@

clean:
	@echo "[Cleaning up...]"
	@-${RM} *.d *.o
