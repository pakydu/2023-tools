#CROSS_COMPILE ?= /usr/bin/arm-buildroot-linux-gnueabi-
CC=$(CROSS_COMPILE)gcc
STRIP=$(CROSS_COMPILE)strip
CURRENT_DIR=$(shell pwd)

test_COMMON_INC ?= $(CURRENT_DIR)/../../common/inc
test_INSTALL_BIN ?= $(CURRENT_DIR)/../../../tmp_install/bin
test_INSTALL_LIB ?= $(CURRENT_DIR)/../../../tmp_install/lib
INCLUDE_CFLAGS=-I$(test_COMMON_INC)
LIB_LDFLAGS=-L$(test_INSTALL_LIB) -lrt -lpthread -lslog

#you can support the slog to syslogd. you can support the slog to send remote by udp.
#ATOS_INCLUDE_CFLAGS+= -DSUPPORT_SYSLOG -DSUPPORT_SENT2REMOTE


TARGET_NAME=test_slog

objs = $(patsubst %.c,%.o,$(wildcard *.c))

all:$(TARGET_NAME)

$(TARGET_NAME):$(objs)
	$(CC) $(INCLUDE_CFLAGS)  -o $(TARGET_NAME)  $(objs) $(LIB_LDFLAGS)
	$(STRIP) $(TARGET_NAME)

%.o:%.c
	@echo "compile $@"
	$(CC)  $(INCLUDE_CFLAGS) -c $<

clean:
	@rm -rf $(TARGET_NAME) 
	@rm -rf $(objs)
