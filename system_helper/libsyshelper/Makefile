CURRENT_DIR=$(shell pwd)
INSPUR_COMMON_INC = $(APP_INSPUR_COMMON_DIR)/inc
INSPUR_INSTALL_LIB = $(APP_INSPUR_TMP_INSTALL_DIR)/lib
#INSPUR_INSTALL_BIN = $(APP_INSPUR_TMP_INSTALL_DIR)/bin
#INSPUR_INSTALL_CFG = $(APP_INSPUR_TMP_INSTALL_DIR)/cfg
#export INSPUR_COMMON_INC INSPUR_INSTALL_LIB INSPUR_INSTALL_BIN INSPUR_INSTALL_CFG




ATOS_CROSS_COMPILE ?= /opt/trendchip/buildroot-gcc1030-glibc232_kernel5_4/bin/arm-buildroot-linux-gnueabi-
CC=$(ATOS_CROSS_COMPILE)gcc -g
STRIP=$(ATOS_CROSS_COMPILE)strip

ATOS_INCLUDE_CFLAGS=-I$(INSPUR_COMMON_INC) -I$(LIB_INSTALL_DIR)
ATOS_LDFLAGS=-L$(INSPUR_INSTALL_LIB)  -L$(LIB_INSTALL_DIR) 


INCLUDE = -w --std=gnu99 -g3 -Wmissing-declarations -I. $(ATOS_INCLUDE_CFLAGS)
LD_FLAGS=$(ATOS_LDFLAGS) -shared -fPIC -lpthread -lslog -L$(TRUNK_DIR)/install_bsp/fs/lib

objs = $(patsubst %.c,%.o,$(wildcard *.c))

ifneq ($(TC_CFLAGS), )
CFLAGS+=$(TC_CFLAGS)
endif

# add the debug for backtrace...
ifneq ($(TCSUPPORT_INSPUR_RELEASE),)
$(info "-------> disable debug mode for release version <---------------")
CFLAGS+= -Os -rdynamic -funwind-tables -ffunction-sections
else
$(info "-------> enable debug mode <---------------")
CFLAGS+= -g -O0 -rdynamic -funwind-tables -ffunction-sections
endif

TARGET_NAME=libsyshelper
all: $(TARGET_NAME)

$(TARGET_NAME):  $(objs)
	$(CC) $(INCLUDE) $(CFLAGS) $(LD_FLAGS) -o $(TARGET_NAME).so *.c
	$(STRIP) $(TARGET_NAME).so
	cp -fapd $(TARGET_NAME).so  $(TRUNK_DIR)/install_bsp/fs/lib

%.o:%.c
	@echo "compile $@"
	$(CC)  $(CFLAGS) $(TC_CFLAGS) $(INCLUDE) -c $<

clean:
	@rm -rf $(TARGET_NAME)
	@rm -rf *.o
