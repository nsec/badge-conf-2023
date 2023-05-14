#!/bin/bash

# SPDX-FileCopyrightText: 2023 NorthSec
#
# SPDX-License-Identifier: MIT

set -eu

#debug
#set -x

# Path to the firmware file
FIRMWARE="binary/firmware.hex"

# Set the path to the patched avrdude binary
AVRDUDE="$HOME/tmp/avrdude/build_linux/src/avrdude"

# Set the usb bus and id numbers of all 4 usbasp
USB0=(003 004)
USB1=(001 009)
USB2=(005 003)
USB3=(000 000)


check_firmware() {
    if [ ! -f "$FIRMWARE" ]; then
        echo "Missing firmware file: $FIRMWARE"
        exit 1
    fi
}

check_avrdude() {
    if ! command -v "$AVRDUDE" >/dev/null; then
        echo "Set the PATH to the patched avrdude binary in \$AVRDUDE"
        exit 1
    fi
}

flash_loop() {
    local usb_bus=$1
    local usb_id=$2

    check_avrdude
    check_firmware

    if test ! -c "/dev/bus/usb/$usb_bus/$usb_id"; then
        echo "No usbasp on usb:$usb_bus:$usb_id"
        exit 1
    fi

    while true; do
        if ! $AVRDUDE -p atmega328pb -c usbasp -P "usb:$usb_bus:$usb_id" -B 10 -U lfuse:r:-:i >/dev/null 2>&1; then
            echo "Waiting for badge on usbasp:$usb_bus:$usb_id"
            sleep 5
            continue
        fi

        error=0

        echo "Setting fuses on usbasp:$usb_bus:$usb_id"
        $AVRDUDE -e -p atmega328pb -c usbasp -P "usb:$usb_bus:$usb_id" -B 10 -U lock:w:0x0f:m -U hfuse:w:0xde:m -U lfuse:w:0xff:m -U efuse:w:0xfd:m || error=1
        if [ $error = 1 ]; then
            echo "Failed to set fuses. Press any key to retry."
            read -r
            continue
        fi

        echo "Flashing on usbasp:$usb_bus:$usb_id"
        $AVRDUDE -p atmega328pb -c usbasp -P "usb:$usb_bus:$usb_id" -B 3Mhz -U "flash:w:$FIRMWARE:i" || error=1
        if [ $error = 1 ]; then
            echo "Failed to flash firmware. Press any key to retry."
            read -r
            continue
        fi

        echo "Flash successful. Press any key to exit."
        read -r
        exit 0
    done
}

master_flasher() {
    local session="master-flasher"
    local window='Master Flasher'

    if ! command -v tmux >/dev/null; then
        echo "This script requires tmux in the PATH."
        exit 1
    fi

    check_avrdude
    check_firmware

    # Create tmux session
    tmux new-session -d -s $session -n "$window"

    # Create panes
    tmux split-window -h -l 50% -t "$session:$window.0"
    tmux split-window -h -l 50% -t "$session:$window.0"
    tmux split-window -h -l 50% -t "$session:$window.2"

    # Start flash script in each pane
    tmux send-keys -t "$session:$window.0" "${BASH_SOURCE[0]} flash0" Enter
    tmux send-keys -t "$session:$window.1" "${BASH_SOURCE[0]} flash1" Enter
    tmux send-keys -t "$session:$window.2" "${BASH_SOURCE[0]} flash2" Enter
    tmux send-keys -t "$session:$window.3" "${BASH_SOURCE[0]} flash3" Enter

    # Attach to tmux session
    tmux a -t "$session:$window"

    exit $?
}

case ${1:-} in
flash0)
    flash_loop "${USB0[@]}"
    ;;
flash1)
    flash_loop "${USB1[@]}"
    ;;
flash2)
    flash_loop "${USB2[@]}"
    ;;
flash3)
    flash_loop "${USB3[@]}"
    ;;
master)
    master_flasher
    ;;
list)
    lsusb | grep 16c0:05dc
    exit 0
    ;;
*)
    echo "Usage: master-flasher [command]"
    echo "  list    List the plugged-in usbasp"
    echo "  master  Flash on all 4 ports"
    echo "  flash0  Flash on port 0"
    echo "  flash1  Flash on port 1"
    echo "  flash2  Flash on port 2"
    echo "  flash3  Flash on port 3"
    exit 0
    ;;
esac

# vim: expandtab tabstop=4 shiftwidth=4
