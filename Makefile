# SPDX-FileCopyrightText: 2023 NorthSec
#
# SPDX-License-Identifier: MIT

ENV = default

VERBOSE = -v

all: build

build:
	pio run -e $(ENV) $(VERBOSE)

flash:
	pio run -e $(ENV) $(VERBOSE) -t upload -v

fuses:
	pio run -e $(ENV) $(VERBOSE) -t fuses

compiledb:
	pio run -e $(ENV) $(VERBOSE) -t compiledb

check:
	pio test -e native_tests $(VERBOSE)

reuse:
	reuse lint

.PHONY: build flash fuses compiledb check reuse
