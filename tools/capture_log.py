#!/usr/bin/env python3
"""Reset the board and save its serial log, without the interactive idf.py monitor.

Run it from the ESP-IDF shell, which provides pyserial:
    python tools/capture_log.py -p /dev/ttyACM0 -s 15 -o pn532.log
"""
import argparse
import sys
import time

import serial


def reset(port):
    # Same sequence as esptool's hard reset: EN low with GPIO0 left high, so it boots the app.
    port.dtr = False
    port.rts = True
    time.sleep(0.1)
    port.rts = False


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("-p", "--port", default="/dev/ttyACM0")
    parser.add_argument("-s", "--seconds", type=float, default=15)
    parser.add_argument("-o", "--output", default="serial.log")
    parser.add_argument("--no-reset", action="store_true", help="capture without resetting")
    args = parser.parse_args()

    with serial.Serial(args.port, 115200, timeout=0.2) as port, open(args.output, "wb") as out:
        if not args.no_reset:
            reset(port)
        end = time.monotonic() + args.seconds
        while time.monotonic() < end:
            data = port.read(1024)
            if data:
                out.write(data)
                sys.stdout.buffer.write(data)
                sys.stdout.flush()
    print(f"\n--- saved to {args.output}", file=sys.stderr)


if __name__ == "__main__":
    main()
