
PROJ_BASE_DIR := $(CURDIR)

TOOLCHAIN     := keil
TOOLCHAIN_DIR := D:/Application/Keil_v5

SDK_ROOT_DIR  	  := $(PROJ_BASE_DIR)/libraries
RTT_ROOT_DIR  	  := $(PROJ_BASE_DIR)/rt-thread
PKGS_ROOT_DIR  	  := $(PROJ_BASE_DIR)/packages

SHELL := /bin/bash

export RTT_CC=$(TOOLCHAIN)
export RTT_EXEC_PATH=$(TOOLCHAIN_DIR)

export RTT_ROOT=$(RTT_ROOT_DIR)
export SDK_ROOT=$(SDK_ROOT_DIR)
export PKGS_DIR=$(PKGS_ROOT_DIR)

.PHONY: FORCE
.PHONY: image rebuild clean gen_mdk5_proj

image:
	@scons -j16

rebuild:
	@scons -c
	@scons -j16

clean:
	@scons -c

menuconfig:
	@scons --menuconfig

gen_mdk5_proj:
	@scons --target=mdk5
