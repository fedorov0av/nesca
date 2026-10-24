# Headless CLI

The CLI runs the scanner without showing the GUI. It uses Qt's `offscreen`
platform and writes structured results as JSON Lines (JSONL).

> Use the CLI only against systems and networks for which you have explicit
> authorization. The CLI intentionally does not ship a public/private target
> guard; responsibility for the target rests with the operator.

## Quick start

```bash
docker build -t nesca:dev .
docker run --rm --network host nesca:dev \
  ./nesca --headless --target 127.0.0.1 --ports 8080 --jsonl -
```

The JSONL stream goes to stdout. Use `--jsonl results.jsonl` to write a file.

## Modes

| Mode | Required input | Meaning |
|---|---|---|
| `ip` (default) | `--target` | IP, range (`a-b`) or CIDR |
| `dns` | `--target` | DNS/TLD scan |
| `import` | `--target-file` | File with ranges/CIDRs |

Import files accept one range or CIDR per line, for example:

```
192.0.2.10-192.0.2.20
192.0.2.0/28
```

Blank lines, `#` comments and `//` comments are ignored. A single IP without
a mask is not accepted by import mode; use `192.0.2.10-192.0.2.10` or
`192.0.2.10/32`.

## Options

| Option | Default | Description |
|---|---|---|
| `--headless` | off | Run without GUI |
| `--mode <ip\|dns\|import>` | `ip` | Scan mode |
| `--target <value>` | — | IP/range/CIDR or DNS target |
| `--target-file <path>` | — | Import target list |
| `--ports <csv>` | common ports | Comma-separated TCP ports |
| `--threads <n>` | `100` | Scanner worker limit |
| `--timeout <sec>` | `3` | Per-connection timeout |
| `--delay <ms>` | `10` | Delay between worker launches |
| `--brute` | off | Enable credential checks |
| `--brute-threads <n>` | `10` | Concurrent credential checks |
| `--userlist <path>` | — | HTTP/Hikvision/RTSP logins |
| `--passlist <path>` | — | HTTP/Hikvision/RTSP passwords |
| `--ftp-userlist <path>` | — | FTP logins |
| `--ftp-passlist <path>` | — | FTP passwords |
| `--wf-userlist <path>` | — | Web-form logins |
| `--wf-passlist <path>` | — | Web-form passwords |
| `--ssh-passlist <path>` | — | SSH `user:pass` entries |
| `--use-default-wordlists` | off | Use bundled wordlists for brute |
| `--jsonl <path\|->` | `-` | JSONL output; `-` means stdout |
| `--workdir <dir>` | `./nesca-run-<ts>` | Results and copied wordlists |
| `--max-runtime <sec>` | `3600` | Hard scan limit; `0` disables |
| `--quiet` | off | Suppress `log` records |
| `--no-shuffle` | off | Disable IP range shuffling |

## Brute-force policy

Brute is opt-in. It is never enabled by `--headless` alone.

- With `--brute` and custom lists, the bundled credential lists are removed
  and only the provided lists are used. Modules without a provided list get
  an empty list and perform no attempts.
- With `--brute --use-default-wordlists`, the bundled lists are used; custom
  lists override individual files.
- Without `--brute`, all credential lists are emptied; discovery still works.

Wordlist format: one entry per line. The legacy loader reads up to 31
characters per entry. The SSH list uses `user:pass` per line.

## JSONL records

One compact JSON object per line. Types:

### `start`

```json
{"type":"start","ts":"2026-10-24T11:24:33.000Z","mode":"ip","target":"192.0.2.0/28","ports":"22,80","threads":10,"brute":true,"workdir":"/opt/nesca/src/nesca-run-..."}
```

### `port`

```json
{"type":"port","ts":"...","ip":"192.0.2.3","port":80,"status":"open","banner":"HTTP/1.1 401 ..."}
```

### `finding`

```json
{"type":"finding","ts":"...","ip":"192.0.2.3","port":80,"module":"http-basic","status":"auth","login":"test","password":"test","banner":"..."}
```

Modules currently emitting findings: `ssh`, `ftp`, `http-basic`, `rtsp`,
`hikvision`, `rvi`, `ipc`.

### `log`

```json
{"type":"log","ts":"...","level":"info","line":"List loader - [OK] (1 hosts)"}
```

### `summary`

```json
{"type":"summary","ts":"...","duration_ms":7805,"ports":4,"findings":3,"logs":0,"exit_code":0,"timed_out":false}
```

## Exit codes

| Code | Meaning |
|---|---|
| `0` | Scan finished |
| `1` | `--max-runtime` exceeded |
| `2` | Invalid arguments, missing wordlists, or setup error |

## Work directory

The CLI creates a work directory (or uses `--workdir`), copies `pwd_lists`
into it, and changes into it before starting the GUI engine. This keeps
`results_*` files and wordlists out of the repository and makes runs
independent. The directory is printed in the `start` record.

## Notes and limitations

- DNS mode is experimental in headless form and has not been lab-tested as
  thoroughly as IP/import modes.
- RTSP is currently discovered as a TCP/HTTP response on port 8554/554; a
  dedicated RTSP `OPTIONS` probe is still a TODO.
- Hikvision/IPC findings require real devices or protocol mocks; they are
  implemented but unverified in the Docker lab.
- The CLI uses the same GUI engine internally; `QT_QPA_PLATFORM=offscreen`
  is set automatically when `--headless` is used and the variable is unset.
