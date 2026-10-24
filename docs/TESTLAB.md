# Nesca test lab

A small, isolated Docker network for authorized testing of the scanner. It
contains HTTP Basic, SSH, FTP and RTSP services with known dummy credentials.

**Never point the scanner at public or third-party ranges.** The lab uses
`172.28.0.0/24`; keep it isolated.

## Start

```bash
cd testlab
docker compose up -d --build
```

Wait for the SSH container to finish installing packages (the first start can
take 20–40 seconds).

## Services

| Host | Service | Credentials |
|---|---|---|
| `172.28.0.2:22` | OpenSSH | `test:test` |
| `172.28.0.3:80` | nginx + HTTP Basic | `test:test` |
| `172.28.0.4:21` | vsftpd | `test:test` |
| `172.28.0.5:8554` | MediaMTX RTSP | no auth (discovery only) |

## Run the scan

From the repository root:

```bash
./testlab/run-scan.sh
```

The script builds `nesca:dev` and runs the headless CLI on the `nesca-lab`
network, scanning `172.28.0.0/29` with the lab wordlists.

Expected records: four `port` records, three `finding` records (SSH, HTTP
Basic, FTP) and a `summary` with `exit_code: 0`.

## Stop

```bash
cd testlab
docker compose down
```

## Notes

- The lab images are third-party and carry their own licenses.
- The credentials are intentionally weak and are for the isolated lab only.
- RTSP is currently detected through its HTTP response; see `docs/CLI.md`.
- Hikvision/IPC need real devices or protocol mocks and are not part of this
  lab.
