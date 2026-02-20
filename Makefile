GO ?= go
BIN_DIR ?= ./bin
BIN_NAME ?= snapsync
BIN_EXT := $(shell $(GO) env GOEXE)
VERSION ?= dev
COMMIT ?= $(shell git rev-parse --short HEAD 2>/dev/null || echo unknown)
DATE ?= $(shell date -u +%Y-%m-%dT%H:%M:%SZ)
LDFLAGS := -X snapsync/internal/buildinfo.Version=$(VERSION) -X snapsync/internal/buildinfo.Commit=$(COMMIT) -X snapsync/internal/buildinfo.Date=$(DATE)

.PHONY: test test-race lint build ci

test:
	$(GO) test ./...

test-race:
	$(GO) test -race ./...

lint:
	golangci-lint run

build:
	mkdir -p $(BIN_DIR)
	$(GO) build -ldflags "$(LDFLAGS)" -o $(BIN_DIR)/$(BIN_NAME)$(BIN_EXT) ./cmd/snapsync

ci: lint test
