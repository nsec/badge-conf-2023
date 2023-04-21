# SPDX-FileCopyrightText: 2023 NorthSec
# SPDX-License-Identifier: MIT

Import("env")

# The default configuration of a factory-fresh ATmega328 is to run at 1MHz
# using internal 8MHz clock and x8 prescaler. This is too slow for the default
# speed of the USBasp, use a slower bit clock of 10us to set the fuses.
env.Append(
    FUSESUPLOADERFLAGS=[
        "-B", "10",
    ]
)
