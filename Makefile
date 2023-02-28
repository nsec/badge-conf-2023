# SPDX-FileCopyrightText: 2023 NorthSec
#
# SPDX-License-Identifier: MIT

ENV = default

all: build

build:
	pio run -e $(ENV)

flash:
	pio run -e $(ENV) -t upload

fuses:
	pio run -e $(ENV) -t fuses

compiledb:
	pio run -e $(ENV) -t compiledb

check:
	pio test -e native_tests

reuse:
	reuse lint

.PHONY: build flash fuses compiledb check reuse
