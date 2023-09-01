
.PHONY: all system_helper

#APP_SLOG_DIR
#APP_INSPUR_TMP_INSTALL_DIR
CURRENT_DIR=$(shell pwd)
APP_INSPUR_TMP_INSTALL_DIR ?= $(CURRENT_DIR)/..
APP_SLOG_DIR ?= $(CURRENT_DIR)
INSPUR_COMMON_INC = $(APP_INSPUR_COMMON_DIR)/inc
INSPUR_INSTALL_LIB = $(APP_INSPUR_TMP_INSTALL_DIR)/lib
INSPUR_INSTALL_BIN = $(APP_INSPUR_TMP_INSTALL_DIR)/bin
INSPUR_INSTALL_CFG = $(APP_INSPUR_TMP_INSTALL_DIR)/cfg
export INSPUR_COMMON_INC INSPUR_INSTALL_LIB INSPUR_INSTALL_BIN INSPUR_INSTALL_CFG


TARGETS:=system_helper install

all: target_show $(TARGETS)


target_show:
	@echo "project list:$(TARGETS)"
	

system_helper:
	@echo " build the system_helper..."
	$(MAKE) -C $(APP_SYSTEMHELPER_DIR)/libsyshelper
	$(MAKE) -C $(APP_SYSTEMHELPER_DIR)/test V=s

system_helper_clean:
	@echo " clean the system_helper..."
	$(MAKE) -C  $(APP_SYSTEMHELPER_DIR)/libsyshelper  clean

install:
	@echo "install the system_helper project..."
	@cp -fapd $(APP_3RD_SYSROOT_DIR)/lib/libcurl.so*  $(TRUNK_DIR)/install_bsp/fs/lib

.PHONY: clean
clean: system_helper_clean 
	@echo "clean the system_helper project..."

