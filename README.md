# SnapSync

SnapSync is a LAN file transfer tool. Phase 2 adds mDNS-based LAN discovery so you can list available receivers and send by peer ID, while keeping direct `host:port` transfer fully supported.

## Requirements

- Go 1.22+

## Build

```bash
make build
```

## Test

```bash
make test
make test-race
make lint
```

## Usage

Receiver with discovery advertisement:

```bash
./bin/snapsync recv --listen :45999 --out ./downloads --accept --name LivingRoomPC
```

List discovered peers:

```bash
./bin/snapsync list --timeout 2s
```

Send by peer ID:

```bash
./bin/snapsync send ./movie.mkv --to a1b2c3d4e5 --timeout 3s
```

Direct fallback (no discovery required):

```bash
./bin/snapsync send ./movie.mkv --to 192.168.1.10:45999
```

Version info:

```bash
./bin/snapsync version
```
