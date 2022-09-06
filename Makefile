GO ?= go
SOURCES = $(shell find . -path './.*' -prune -o \( \( -name '*.go' -o -name '*.c' \) -a ! -name '*_test.go' \) -print)
PROJECT := github.com/cdoern/LS3

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