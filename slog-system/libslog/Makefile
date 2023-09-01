#CROSS_COMPILE ?= /usr/bin/arm-buildroot-linux-gnueabi-
CC=$(CROSS_COMPILE)gcc
STRIP=$(CROSS_COMPILE)strip
CURRENT_DIR=$(shell pwd)
test_COMMON_INC ?= $(CURRENT_DIR)/../../common/inc
test_INSTALL_BIN ?= $(CURRENT_DIR)/../../tmp_install/bin
test_INSTALL_LIB ?= $(CURRENT_DIR)/../../tmp_install/lib
INCLUDE_CFLAGS=-I$(test_COMMON_INC) 
LIB_LDFLAGS=-L$(test_INSTALL_LIB) -lrt -lpthread

#you can support the slog to syslogd. you can support the slog to send remote by udp.
#ATOS_INCLUDE_CFLAGS+= -DSUPPORT_SYSLOG -DSUPPORT_SENT2REMOTE


TARGET_NAME=libslog

src = $(wildcard *.c)
objs = $(patsubst %.c,%.o,$(wildcard *.c))

all:$(TARGET_NAME)

$(TARGET_NAME):$(objs)
	$(CC) -shared -fPIC -o $@.so  $^ $(LIB_LDFLAGS)
	$(STRIP) $(TARGET_NAME).so
	@echo "install to $(test_INSTALL_LIB)"
	@cp $(TARGET_NAME).so $(test_INSTALL_LIB)

%.o:%.c
	@echo "comple the $^, $(CURRENT_DIR),$(test_COMMON_INC) "
	$(CC) $(INCLUDE_CFLAGS) -fPIC -c $<

clean:
	@rm -rf $(TARGET_NAME).so
	@rm -rf $(objs)
	@rm -rf $(test_INSTALL_INC)/libslog.h
