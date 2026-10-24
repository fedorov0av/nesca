# Benchmark: baseline vs thread pool

The `perf/threadpool` branch ports the safe part of the `dnet44/nesca-remaked`
modernization: a fixed worker pool and atomics for counters. It does **not**
remove the legacy delays or switch to `std::string_view`.

## Method

- Host: Ubuntu 26.04, Docker 29.9, 8 vCPU (container limits may vary).
- Lab: `testlab/docker-compose.yml`, network `172.28.0.0/24`.
- Target: `172.28.0.0/29`, ports `21,22,80,8554`.
- CLI: `--threads 10 --timeout 3 --brute --brute-threads 5`,
  wordlists from `testlab/wordlists`.
- Each run starts a fresh container; the scanner process exits after the scan.
- Result: `summary.duration_ms` from the JSONL stream.

## Results

| Revision | Run 1 | Run 2 | Run 3 |
|---|---:|---:|---:|
| `merge-dev` baseline (per-IP threads) | 7805 ms | — | — |
| `perf/threadpool` (fixed pool) | 7594 ms | 7595 ms | 7595 ms |

The baseline was measured once before the pool change; the pool branch was
measured three times. The difference is small for an 8-address lab. The main benefit is structural:
the pool is created once, workers are joined during cleanup, and counters
(`found`, `saved`, `Alive`, `baCount`, `ipCounter`, etc.) are atomic.

## Correctness checks

- Full lab scan: 4 open ports, 3 auth findings, `exit_code: 0`.
- Repeated runs: 3/3 identical summaries, no crashes.
- `--max-runtime 1`: exits with code 1 and a `timed_out: true` summary;
  workers are stopped/joined before process exit.
- Invalid ports: exits cleanly with an error log.

## What was not ported

- `std::string_view` parsing in `Utils`.
- Removal of the `Sleep` delays in scan/brute paths.
- C++17 build mode.

These need separate benchmarks and regression tests before merging.
