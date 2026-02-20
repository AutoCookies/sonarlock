# SnapSync

SnapSync is a LAN file transfer tool. Phase 1 provides a real single-connection TCP sender/receiver for manual host:port transfers on local networks.

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

Receiver:

```bash
./bin/snapsync recv --listen :45999 --out ./downloads --accept
```

Sender:

```bash
./bin/snapsync send ./big.iso --to 192.168.1.10:45999
```

Version info:

```bash
./bin/snapsync version
```
