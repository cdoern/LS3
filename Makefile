GO ?= go
SOURCES = $(shell find . -path './.*' -prune -o \( \( -name '*.go' -o -name '*.c' \) -a ! -name '*_test.go' \) -print)
PROJECT := github.com/cdoern/LS3
BACKING_FILE := $(PWD)/testing.img
SIZE_IN_MB := 1
MOUNTPOINT := $(PWD)/testmount

ifeq ($(GOPATH),)
export GOPATH := $(HOME)/go
unexport GOBIN
endif
FIRST_GOPATH := $(firstword $(subst :, ,$(GOPATH)))
GOPKGDIR := $(FIRST_GOPATH)/src/$(PROJECT)
GOPKGBASEDIR ?= $(shell dirname "$(GOPKGDIR)")

GOBIN := $(shell $(GO) env GOBIN)
ifeq ($(GOBIN),)
GOBIN := $(FIRST_GOPATH)/bin
endif

export PATH := $(PATH):$(GOBIN)

GOCMD = CGO_ENABLED=$(CGO_ENABLED) GOOS=$(GOOS) GOARCH=$(GOARCH) $(GO)
BUILDFLAGS := -mod=vendor $(BUILDFLAGS)
GO_LDFLAGS:= $(shell if $(GO) version|grep -q gccgo ; then echo "-gccgoflags"; else echo "-ldflags"; fi)

.gopathok:
ifeq ("$(wildcard $(GOPKGDIR))","")
	mkdir -p "$(GOPKGBASEDIR)"
	ln -sfn "$(CURDIR)" "$(GOPKGDIR)"
endif
	touch $@

all: bin/ls3 bin/mkfs.ls3 modules

bin/ls3: .gopathok $(SOURCES) go.mod go.sum
	$(GOCMD) build \
		$(BUILDFLAGS) \
		$(GO_LDFLAGS) '' \
		-tags "" \
		-o $@ ./cmd/

bin/mkfs.ls3: .gopathok $(SOURCES) go.mod go.sum
	$(GOCMD) build \
		$(BUILDFLAGS) \
		$(GO_LDFLAGS) '' \
		-tags "" \
		-o $@ ./cmdMkfs/



.PHONY: mkfs.ls3
mkfs.ls3: bin/mkfs.ls3

.PHONY: ls3
ls3: bin/ls3

.PHONY: vendor
vendor:
	go mod init LS3
	go mod tidy
	go mod vendor


obj-m += ls3_fs.o
CONFIG_MODULE_SIG=n
ccflags-y += -Wno-declaration-after-statement -Wno-unused-function

.PHONY: modules
modules: 
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD)/kernel modules

.PHONY: clean
clean: 
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD)/kernel clean

.PHONY: zero
zero:
	rm -f $(BACKING_FILE)
	dd if=/dev/zero of=$(BACKING_FILE) bs=1024 count=$(shell expr $(SIZE_IN_MB) \* 1024)

.PHONY: insmod
insmod:
	sync # to ensure files are stored before crashing
	sudo insmod kernel/ls3_fs.ko backing_file=$(BACKING_FILE) verbose=1

.PHONY: rmmod
rmmod:
	sudo rmmod ls3_fs

.PHONY: format
format:
	bin/mkfs.ls3 $(BACKING_FILE)

.PHONY: mount
mount:
	mkdir -p $(MOUNTPOINT)
	sudo mount -t ls3 -o loop $(BACKING_FILE) $(MOUNTPOINT)

.PHONY: umount
umount:
	sudo umount $(MOUNTPOINT) || true


.PHONY: starttest
starttest: zero insmod format mount

.PHONY: endtest
starttest: umount rmmod
