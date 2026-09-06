#!/bin/sh
# Serial console for CB2S / BK7231N hello-world test.
#
# Uses the SAME UART wiring as flashing (RX1/TX1 on the module, see
# doc/cb2s-back.png) - no rewiring needed after flashing.
#
# Note: the SDK's PR_NOTICE debug log output is normally routed to the
# module's second UART (RX2/TX2), not RX1/TX1. On this wiring you may see
# only bootloader chatter at power-on/reset, and nothing once the app is
# running. If the console looks silent after boot, that confirms app logs
# are going out RX2/TX2 instead - rewire there to see them.
#
# Note: the CH340 adapter can re-enumerate to a different /dev/ttyUSBn
# after a flash (e.g. ttyUSB0 -> ttyUSB1). If this script can't open the
# device, check `ls /dev/serial/by-id/` or `ls /dev/ttyUSB*` for the
# current name and pass it explicitly.
#
# Usage: ./serial-console.sh [/dev/ttyUSBx] [baud]
DEVICE="$1"
BAUD="${2:-115200}"

if [ -z "$DEVICE" ]; then
    DEVICE=$(ls /dev/ttyUSB* 2>/dev/null | head -1)
    if [ -z "$DEVICE" ]; then
        echo "No /dev/ttyUSB* device found. Is the adapter plugged in?" >&2
        exit 1
    fi
    echo "No device given, auto-detected: $DEVICE"
fi

echo "Opening $DEVICE at $BAUD baud (Ctrl-] to exit)..."
exec python3 -m serial.tools.miniterm "$DEVICE" "$BAUD" --raw
