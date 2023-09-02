
MODULE_NAME := t2_fan 

SRC_DIR := $(PWD)

KERNEL_HEADERS := /lib/modules/$(shell uname -r)/build

EXTRA_CFLAGS += -Wall

obj-m += $(MODULE_NAME).o

all:
	$(MAKE) -C $(KERNEL_HEADERS) M=$(SRC_DIR) modules

clean:
	$(MAKE) -C $(KERNEL_HEADERS) M=$(SRC_DIR) clean

install:
	$(MAKE) -C $(KERNEL_HEADERS) M=$(SRC_DIR) modules_install

.PHONY: all clean install
