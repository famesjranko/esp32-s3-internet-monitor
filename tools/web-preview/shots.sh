#!/usr/bin/env bash
# Starts the dashboard and portal previews, captures screenshots into out/, stops both.
set -euo pipefail
cd "$(dirname "$0")"
# PLAYWRIGHT_DIR: a directory whose node_modules holds playwright (no default).
[ -n "${PLAYWRIGHT_DIR:-}" ] && [ -d "$PLAYWRIGHT_DIR/node_modules/playwright" ] || { echo "Set PLAYWRIGHT_DIR to a directory with node_modules/playwright" >&2; exit 1; }
export PLAYWRIGHT_DIR
mkdir -p out
rm -f out/*.png

build/preview --port 8091 >out/dashboard.log 2>&1 &
dash=$!
build/preview --port 8092 --portal >out/portal.log 2>&1 &
portal=$!
trap 'kill $dash $portal 2>/dev/null || true' EXIT
sleep 1

node shots.mjs http://127.0.0.1:8091/ http://127.0.0.1:8092/ out
ls out/*.png
