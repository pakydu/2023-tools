
.PHONY: all libslog slogd test_slog

#APP_SLOG_DIR
#APP_test_TMP_INSTALL_DIR
CURRENT_DIR=$(shell pwd)
APP_test_TMP_INSTALL_DIR ?= $(CURRENT_DIR)/tmp_install
APP_test_COMMON_DIR ?= $(CURRENT_DIR)/common
APP_SLOG_DIR ?= $(CURRENT_DIR)
test_COMMON_INC = $(APP_test_COMMON_DIR)/inc
test_INSTALL_LIB = $(APP_test_TMP_INSTALL_DIR)/lib
test_INSTALL_BIN = $(APP_test_TMP_INSTALL_DIR)/bin
test_INSTALL_CFG = $(APP_test_TMP_INSTALL_DIR)/cfg
export test_COMMON_INC test_INSTALL_LIB test_INSTALL_BIN test_INSTALL_CFG


TARGETS:=libslog slogd test_slog install

all: target_show $(TARGETS)


target_show:
	@echo "project list:$(TARGETS)"
	
libslog:
	@echo " build the libslog...$(APP_test_COMMON_DIR), $(test_COMMON_INC)"
	$(MAKE) -C $(APP_SLOG_DIR)/libslog

libslog_clean:
	@echo " clean the libslog..."
	$(MAKE) -C  $(APP_SLOG_DIR)/libslog  clean
	@rm -rf $(test_INSTALL_LIB)/libslog.so*

slogd: libslog
	@echo " build the slogd..."
	$(MAKE) -C $(APP_SLOG_DIR)/slogd

slogd_clean:
	@echo " clean the slogd..."
	$(MAKE) -C  $(APP_SLOG_DIR)/slogd  clean
	@rm -rf $(test_INSTALL_BIN)/slogd

test_slog: libslog
	@echo " build the test_slog..."
	$(MAKE) -C $(APP_SLOG_DIR)/libslog/test

test_slog_clean:
	@echo " clean the test_slog..."
	$(MAKE) -C $(APP_SLOG_DIR)/libslog/test clean
	@rm -rf $(test_INSTALL_BIN)/test_slog

install:
	@echo "install the slog project..."

.PHONY: clean
clean: libslog_clean slogd_clean 
	@echo "clean the slog project..."

