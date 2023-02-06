GO ?= go
SOURCES = $(shell find . -path './.*' -prune -o \( \( -name '*.go' -o -name '*.c' \) -a ! -name '*_test.go' \) -print)
PROJECT := github.com/cdoern/LS3
BACKING_FILE := /home/charliedoern/Documents/testing.txt

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

bin/ls3: .gopathok $(SOURCES) go.mod go.sum
	$(GOCMD) build \
		$(BUILDFLAGS) \
		$(GO_LDFLAGS) '' \
		-tags "" \
		-o $@ ./cmd/

.PHONY: ls3
ls3: bin/ls3

.PHONY: vendor
vendor:
	# go mod init
	go mod tidy
	go mod vendor


obj-m += ls3.o
CONFIG_MODULE_SIG=n

.PHONY: c
c: 
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD)/kernel modules

.PHONY: clean
clean: 
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD)/kernel clean

.PHONY: zero
zero:
	dd if=/dev/zero of=$(BACKING_FILE) bs=1000 count=1000000

.PHONY: insmod
insmod:
	sudo insmod kernel/ls3.ko backing_file=$(BACKING_FILE) verbose=1

.PHONY: rmmod
rmmod:
	sudo rmmod ls3
