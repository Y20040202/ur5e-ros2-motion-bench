#!/usr/bin/env bash
set -eu

if [ -r /etc/os-release ]; then
    awk -F= '$1 == "PRETTY_NAME" {print $0}' /etc/os-release
fi
uname -sm
df -h "$PWD"
if command -v free >/dev/null 2>&1; then
    free -h
fi
if command -v python3 >/dev/null 2>&1; then
    python3 --version
fi
if command -v conda >/dev/null 2>&1; then
    conda --version
fi
if command -v nvidia-smi >/dev/null 2>&1; then
    nvidia-smi --query-gpu=name,memory.total,driver_version --format=csv ||
        printf '%s\n' 'nvidia-smi could not query the GPU'
else
    printf '%s\n' 'nvidia-smi unavailable in this environment'
fi
