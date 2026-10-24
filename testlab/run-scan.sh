#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")/.."

docker build -t nesca:dev .

exec docker run --rm --network nesca-lab \
    -v "$PWD/testlab/wordlists:/wordlists:ro" \
    nesca:dev ./nesca --headless \
    --target 172.28.0.0/29 \
    --ports 21,22,80,8554 \
    --threads 10 \
    --timeout 3 \
    --brute \
    --brute-threads 5 \
    --userlist /wordlists/login.txt \
    --passlist /wordlists/pass.txt \
    --ftp-userlist /wordlists/ftplogin.txt \
    --ftp-passlist /wordlists/ftppass.txt \
    --ssh-passlist /wordlists/sshpass.txt \
    --max-runtime 120 \
    --jsonl -
