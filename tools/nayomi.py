#!/usr/bin/env python3
"""
Nayomi host CLI.

No pyserial dependency: this uses the Linux tty directly, so the normal
workflow is simply:

    ./tools/nayomi.py status
    ./tools/nayomi.py hall
    ./tools/nayomi.py info
    ./tools/nayomi.py monitor

The device path defaults to the stable /dev/serial/by-id Nayomi link.
"""

import argparse
import os
import select
import struct
import sys
import termios
import time

MAGIC = b"NY"
VERSION = 1
MAX_PAYLOAD = 128

CMD_PING = 0x01
CMD_INFO = 0x02
CMD_STATUS = 0x03
CMD_HALL = 0x04
CMD_RESET = 0x05
RSP = 0x80
EVT_HALL = 0x90


def crc16(data: bytes) -> int:
    crc = 0xFFFF
    for byte in data:
        crc ^= byte << 8
        for _ in range(8):
            crc = ((crc << 1) ^ 0x1021) & 0xFFFF if crc & 0x8000 else (crc << 1) & 0xFFFF
    return crc


def frame(msg_type: int, sequence: int, payload: bytes = b"") -> bytes:
    header = struct.pack("<BBB H", VERSION, msg_type, sequence, len(payload))
    body = header + payload
    return MAGIC + body + struct.pack("<H", crc16(body))


class Decoder:
    def __init__(self):
        self.buf = bytearray()

    def feed(self, data: bytes):
        self.buf.extend(data)
        frames = []

        while True:
            pos = self.buf.find(MAGIC)
            if pos < 0:
                self.buf.clear()
                break
            if pos:
                del self.buf[:pos]
            if len(self.buf) < 9:
                break

            version, msg_type, seq, length = struct.unpack_from("<BBB H", self.buf, 2)
            total = 2 + 5 + length + 2

            if length > MAX_PAYLOAD:
                del self.buf[:2]
                continue
            if len(self.buf) < total:
                break

            body = bytes(self.buf[2:7 + length])
            received = struct.unpack_from("<H", self.buf, 7 + length)[0]
            del self.buf[:total]

            if version != VERSION or crc16(body) != received:
                continue

            payload = body[5:]
            frames.append((msg_type, seq, payload))

        return frames


def open_tty(path):
    fd = os.open(path, os.O_RDWR | os.O_NOCTTY | os.O_NONBLOCK)
    attrs = termios.tcgetattr(fd)

    # Raw 8N1. CDC ACM doesn't really care about baud rate, but 115200 is
    # conventional and keeps terminal tools happy.
    cfmakeraw = getattr(termios, "cfmakeraw", None)
    if cfmakeraw:
        cfmakeraw(attrs)
    else:
        attrs[0] = 0
        attrs[1] = 0
        attrs[2] = termios.CS8 | termios.CREAD | termios.CLOCAL
        attrs[3] = 0

    attrs[2] = (attrs[2] & ~termios.CSIZE) | termios.CS8 | termios.CREAD | termios.CLOCAL
    attrs[4] = termios.B115200
    attrs[5] = termios.B115200
    termios.tcsetattr(fd, termios.TCSANOW, attrs)
    return fd


class Nayomi:
    def __init__(self, path):
        self.fd = open_tty(path)
        self.decoder = Decoder()
        self.sequence = 0

    def close(self):
        os.close(self.fd)

    def send(self, msg_type, payload=b""):
        seq = self.sequence
        self.sequence = (self.sequence + 1) & 0xFF
        packet = frame(msg_type, seq, payload)
        offset = 0

        while offset < len(packet):
            try:
                written = os.write(self.fd, packet[offset:])
                if written > 0:
                    offset += written
            except BlockingIOError:
                _, writable, _ = select.select([], [self.fd], [], 1.0)
                if not writable:
                    raise TimeoutError("timed out writing to Nayomi")

        return seq

    def receive(self, timeout=1.0):
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            remaining = max(0.0, deadline - time.monotonic())
            readable, _, _ = select.select([self.fd], [], [], remaining)
            if not readable:
                break
            try:
                data = os.read(self.fd, 512)
            except BlockingIOError:
                continue
            if data:
                yield from self.decoder.feed(data)

    def request(self, msg_type, timeout=1.0):
        seq = self.send(msg_type)
        deadline = time.monotonic() + timeout

        while time.monotonic() < deadline:
            for rx_type, rx_seq, payload in self.receive(deadline - time.monotonic()):
                if rx_seq == seq and rx_type == (msg_type | RSP):
                    return payload

        raise TimeoutError("Nayomi did not answer the request")


def u16(payload, offset=0):
    return struct.unpack_from("<H", payload, offset)[0]


def print_info(payload):
    api = struct.unpack_from("<H", payload, 0)[0]
    rest = payload[2:].split(b"\0")
    mcu = rest[0].decode(errors="replace")
    product = rest[1].decode(errors="replace") if len(rest) > 1 else "?"
    print(f"Product : {product}")
    print(f"MCU     : {mcu}")
    print(f"API     : {api}")


def print_status(payload):
    raw, mv = struct.unpack_from("<HH", payload, 0)
    usb = bool(payload[4])
    frames, crc_errors, malformed = struct.unpack_from("<III", payload, 5)

    print(f"USB       : {'connected' if usb else 'disconnected'}")
    print(f"Hall 1    : {raw:4d} raw  {mv:4d} mV")
    print(f"RX frames : {frames}")
    print(f"CRC errors: {crc_errors}")
    print(f"Malformed : {malformed}")


def monitor(dev):
    print("Nayomi live monitor — Ctrl-C to exit")
    print("Hall 1        USB       RX frames   CRC   malformed   rate")
    print("-------------------------------------------------------------")

    last_time = time.monotonic()
    packets = 0
    rate = 0.0
    last_status = 0.0

    try:
        while True:
            now = time.monotonic()
            if now - last_status >= 1.0:
                dev.send(CMD_STATUS)
                last_status = now

            for msg_type, seq, payload in dev.receive(0.1):
                if msg_type == EVT_HALL and len(payload) >= 4:
                    raw, mv = struct.unpack_from("<HH", payload)
                    packets += 1
                    elapsed = time.monotonic() - last_time
                    if elapsed >= 1.0:
                        rate = packets / elapsed
                        packets = 0
                        last_time = time.monotonic()

                    # Status counters are refreshed independently; keep the
                    # live line useful even if the status response is delayed.
                    print(
                        f"\r\x1b[KHall 1: {raw:4d} raw / {mv:4d} mV"
                        f"   telemetry: {rate:5.1f} Hz",
                        end="",
                        flush=True,
                    )
                elif msg_type == (CMD_STATUS | RSP):
                    raw, mv = struct.unpack_from("<HH", payload, 0)
                    usb = "ON" if payload[4] else "OFF"
                    frames, crc_errors, malformed = struct.unpack_from("<III", payload, 5)
                    print(
                        f"\r\x1b[KHall 1: {raw:4d} raw / {mv:4d} mV"
                        f"   USB: {usb}   RX: {frames}"
                        f"   CRC: {crc_errors}   bad: {malformed}"
                        f"   telemetry: {rate:5.1f} Hz",
                        end="",
                        flush=True,
                    )
    except KeyboardInterrupt:
        print()


def main():
    parser = argparse.ArgumentParser(description="Nayomi keypad command-line tool")
    parser.add_argument(
        "--device",
        default="/dev/serial/by-id/usb-Nayomi_Keypad_23EE39680516-if00",
        help="CDC device path",
    )
    sub = parser.add_subparsers(dest="command", required=True)
    sub.add_parser("ping")
    sub.add_parser("info")
    sub.add_parser("status")
    sub.add_parser("hall")
    sub.add_parser("monitor")
    sub.add_parser("reset")

    args = parser.parse_args()
    dev = Nayomi(args.device)

    try:
        if args.command == "ping":
            print(dev.request(CMD_PING).decode(errors="replace"))

        elif args.command == "info":
            print_info(dev.request(CMD_INFO))

        elif args.command == "status":
            print_status(dev.request(CMD_STATUS))

        elif args.command == "hall":
            payload = dev.request(CMD_HALL)
            raw, mv = struct.unpack_from("<HH", payload)
            print(f"Hall 1: {raw} raw / {mv} mV")

        elif args.command == "monitor":
            monitor(dev)

        elif args.command == "reset":
            print(dev.request(CMD_RESET).decode(errors="replace"))

    except TimeoutError as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1
    finally:
        dev.close()

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
