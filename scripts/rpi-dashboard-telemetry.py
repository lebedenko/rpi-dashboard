#!/usr/bin/env python3
"""Dependency-free Linux telemetry sender for rpi-dashboard protocol v1."""

import argparse
import os
import platform
import socket
import struct
import time
import uuid
from pathlib import Path

VERSION = 1


def _head(major, value):
    if value < 24: return bytes([(major << 5) | value])
    if value < 256: return bytes([(major << 5) | 24, value])
    if value < 65536: return bytes([(major << 5) | 25]) + struct.pack(">H", value)
    if value < 4294967296: return bytes([(major << 5) | 26]) + struct.pack(">I", value)
    return bytes([(major << 5) | 27]) + struct.pack(">Q", value)


def cbor(value):
    if value is None: return b"\xf6"
    if value is False: return b"\xf4"
    if value is True: return b"\xf5"
    if isinstance(value, int): return _head(0, value) if value >= 0 else _head(1, -1 - value)
    if isinstance(value, float): return b"\xfb" + struct.pack(">d", value)
    if isinstance(value, bytes): return _head(2, len(value)) + value
    if isinstance(value, str):
        raw = value.encode("utf-8"); return _head(3, len(raw)) + raw
    if isinstance(value, (list, tuple)): return _head(4, len(value)) + b"".join(cbor(item) for item in value)
    if isinstance(value, dict):
        pairs = sorted(((cbor(key), cbor(item)) for key, item in value.items()), key=lambda pair: (len(pair[0]), pair[0]))
        return _head(5, len(pairs)) + b"".join(key + item for key, item in pairs)
    raise TypeError("unsupported CBOR value")


def decode_registration(data, expected_device, expected_instance):
    offset = 0
    def read():
        nonlocal offset
        if offset >= len(data): raise ValueError("truncated registration response")
        initial = data[offset]; offset += 1; major, extra = initial >> 5, initial & 31
        if extra < 24: size = extra
        elif extra == 24: size = data[offset]; offset += 1
        elif extra == 25: size = struct.unpack_from(">H", data, offset)[0]; offset += 2
        elif extra == 26: size = struct.unpack_from(">I", data, offset)[0]; offset += 4
        elif extra == 27: size = struct.unpack_from(">Q", data, offset)[0]; offset += 8
        else: raise ValueError("indefinite CBOR is unsupported")
        if major == 0: return size
        if major in (2, 3):
            if size > 512 or offset + size > len(data): raise ValueError("invalid response field")
            raw = data[offset:offset + size]; offset += size
            return raw if major == 2 else raw.decode("utf-8")
        if major == 5:
            if size > 16: raise ValueError("response is too large")
            return {read(): read() for _ in range(size)}
        if major == 7 and extra in (20, 21): return extra == 21
        raise ValueError("unsupported response value")
    result = read()
    if offset != len(data) or not isinstance(result, dict): raise ValueError("invalid registration response")
    if result.get("version") != VERSION: raise RuntimeError("unsupported protocol")
    if result.get("type") != "registration_result" or result.get("device_id") != expected_device.bytes or result.get("instance_id") != expected_instance.bytes or not isinstance(result.get("accepted"), bool):
        raise ValueError("invalid registration response")
    if not result["accepted"]: raise PermissionError(result.get("reason", "registration rejected"))


def read_text(path):
    try: return Path(path).read_text(encoding="utf-8", errors="replace").strip()
    except OSError: return ""


def device_id(path=None):
    target = Path(path) if path else Path(os.environ.get("XDG_DATA_HOME", Path.home() / ".local/share")) / "rpi-dashboard/rpi-dashboard-agent/device-id"
    try:
        existing = uuid.UUID(read_text(target))
        if existing.int: return existing
    except (ValueError, OSError): pass
    value = uuid.uuid4(); target.parent.mkdir(parents=True, exist_ok=True); target.write_text(str(value) + "\n", encoding="ascii"); os.chmod(target, 0o600); return value


def system_info():
    release = {}
    for line in read_text("/etc/os-release").splitlines():
        if "=" in line:
            key, raw = line.split("=", 1); release[key] = raw.strip().strip('"')
    cpuinfo = read_text("/proc/cpuinfo")
    fields = {}
    physical = set()
    for block in cpuinfo.split("\n\n"):
        values = dict(line.split(":", 1) for line in block.splitlines() if ":" in line)
        fields.update({key.strip(): val.strip() for key, val in values.items()})
        if "physical id" in values and "core id" in values: physical.add((values["physical id"].strip(), values["core id"].strip()))
    memory = next((int(line.split()[1]) * 1024 for line in read_text("/proc/meminfo").splitlines() if line.startswith("MemTotal:")), None)
    def present(mapping): return {key: val for key, val in mapping.items() if val not in (None, "")}
    return present({
        "host": present({"host_name": socket.gethostname()}),
        "os": present({"os_family": platform.system(), "os_id": release.get("ID"), "os_version": release.get("VERSION_ID"), "os_pretty_name": release.get("PRETTY_NAME")}),
        "kernel": present({"kernel_type": platform.system(), "kernel_version": platform.release()}),
        "hardware": present({"manufacturer": read_text("/sys/class/dmi/id/sys_vendor"), "model": read_text("/sys/class/dmi/id/product_name"), "board_revision": read_text("/sys/class/dmi/id/board_version")}),
        "cpu": present({"architecture": platform.machine(), "vendor": fields.get("vendor_id"), "model": fields.get("model name"), "logical_cpu_count": os.cpu_count(), "physical_core_count": len(physical) or None}),
        "memory": present({"total_bytes": memory})})


class Collector:
    def __init__(self): self.previous_cpu = None; self.previous_network = {}; self.previous_time = None
    def collect(self):
        now = time.monotonic(); result = {}; cpu = {}; memory = {}; system = {}
        stat = read_text("/proc/stat").splitlines()
        if stat and stat[0].startswith("cpu "):
            ticks = [int(value) for value in stat[0].split()[1:]]; idle = ticks[3] + (ticks[4] if len(ticks) > 4 else 0); total = sum(ticks)
            if self.previous_cpu and total > self.previous_cpu[0]: cpu["usage_ratio"] = max(0.0, min(1.0, 1.0 - (idle - self.previous_cpu[1]) / (total - self.previous_cpu[0])))
            self.previous_cpu = (total, idle)
        logical = []
        for path in sorted(Path("/sys/devices/system/cpu").glob("cpu[0-9]*/cpufreq/scaling_cur_freq")):
            try: logical.append({"name": path.parts[-3], "frequency_hz": int(read_text(path)) * 1000})
            except ValueError: pass
        if logical: cpu["logical_cpus"] = logical
        temperatures = []
        for path in Path("/sys/class/thermal").glob("thermal_zone*/temp"):
            try: temperatures.append(float(read_text(path)) / 1000.0)
            except ValueError: pass
        if temperatures: cpu["temperature_celsius"] = max(temperatures)
        values = {}
        for line in read_text("/proc/meminfo").splitlines():
            if ":" in line:
                key, raw = line.split(":", 1)
                try: values[key] = int(raw.split()[0]) * 1024
                except (ValueError, IndexError): pass
        for source, target in (("MemTotal","total_bytes"),("MemAvailable","available_bytes"),("SwapTotal","swap_total_bytes"),("SwapFree","swap_available_bytes")):
            if source in values: memory[target] = values[source]
        try: system["uptime_seconds"] = float(read_text("/proc/uptime").split()[0])
        except (ValueError, IndexError): pass
        try:
            loads = [float(item) for item in read_text("/proc/loadavg").split()[:3]]
            system.update(zip(("load_average_1m","load_average_5m","load_average_15m"), loads))
        except ValueError: pass
        networks = []
        elapsed = now - self.previous_time if self.previous_time is not None else None
        for line in read_text("/proc/net/dev").splitlines()[2:]:
            if ":" not in line: continue
            name, raw = line.split(":", 1); name = name.strip()
            if name == "lo": continue
            columns = raw.split()
            try: rx, tx = int(columns[0]), int(columns[8])
            except (ValueError, IndexError): continue
            entry = {"name": name, "rx_bytes": rx, "tx_bytes": tx}
            if elapsed and elapsed > 0 and name in self.previous_network:
                old_rx, old_tx = self.previous_network[name]
                if rx >= old_rx: entry["rx_bytes_per_second"] = (rx - old_rx) / elapsed
                if tx >= old_tx: entry["tx_bytes_per_second"] = (tx - old_tx) / elapsed
            self.previous_network[name] = (rx, tx); networks.append(entry)
        self.previous_time = now
        try:
            disk = os.statvfs("/"); total = disk.f_blocks * disk.f_frsize; available = disk.f_bavail * disk.f_frsize
            result["storage_volumes"] = [{"mount_point":"/", "device_name":"root", "primary":True, "read_only":False, "total_bytes":total, "available_bytes":available}]
        except OSError: pass
        if cpu: result["cpu"] = cpu
        if memory: result["memory"] = memory
        if system: result["system"] = system
        if networks: result["network_interfaces"] = networks
        return result


def envelope(kind, device, instance): return {"version": VERSION, "type": kind, "device_id": device.bytes, "instance_id": instance.bytes}


def main(argv=None):
    parser = argparse.ArgumentParser()
    parser.add_argument("--dashboard-host", required=True); parser.add_argument("--dashboard-port", type=int, default=51337)
    parser.add_argument("--interval", type=int, default=1); parser.add_argument("--display-name")
    parser.add_argument("--device-id", type=uuid.UUID); parser.add_argument("--once", action="store_true")
    args = parser.parse_args(argv)
    if not 1 <= args.dashboard_port <= 65535: parser.error("--dashboard-port must be 1..65535")
    if not 1 <= args.interval <= 5: parser.error("--interval must be 1..5")
    identity, instance = args.device_id or device_id(), uuid.uuid4(); info = system_info()
    hello = envelope("hello", identity, instance); hello.update({"display_name": args.display_name or socket.gethostname(), "interval_s":args.interval, "system_info":info})
    collector, sequence, last_hello = Collector(), 0, 0.0
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM); sock.settimeout(2.0)
    while True:
        try:
            address = socket.getaddrinfo(args.dashboard_host, args.dashboard_port, socket.AF_INET, socket.SOCK_DGRAM)[0][4]
            now = time.monotonic()
            if now - last_hello >= 10:
                sock.sendto(cbor(hello), address); response, _ = sock.recvfrom(2048); decode_registration(response, identity, instance); last_hello = now
            snapshot = envelope("snapshot", identity, instance); snapshot.update({"interval_s":args.interval, "sequence":sequence, "system_info":info})
            try:
                metrics = collector.collect()
                if metrics: snapshot["metrics"] = metrics
            except (OSError, ValueError): pass
            sock.sendto(cbor(snapshot), address); sequence += 1
            if args.once: return 0
            time.sleep(args.interval)
        except (socket.gaierror, socket.timeout, OSError) as error:
            if args.once: raise SystemExit(str(error))
            time.sleep(args.interval)
        except (ValueError, PermissionError, RuntimeError) as error: raise SystemExit(str(error))


if __name__ == "__main__": raise SystemExit(main())
