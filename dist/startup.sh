#!/bin/sh

# Push Fight Analysis server startup script for the Docker container.

set -e

# Start lookup server first, so that it is ready when nginx runs.
# This avoids 502 Bad Gateway errors short after startup, which matters when
# running the container on Google Cloud Run which may shut down the server
# when it's idle.
./lookup-min-http-server.py \
    --ipv4 \
    --host=127.0.0.1 \
    --port=8003 \
    --lookup=./lookup-min \
    --minimized=minimized.bin.xz \
    &
LOOKUP_SERVER_PID=$!

# Wait for Python script to start serving:
TEST_URL=http://127.0.0.1:8003/perms/.OX.....oXx....oOx.....OX.
TEST_CONTENT='"status": "T"'
if ! curl --no-progress-meter --retry 10 --retry-connrefused "$TEST_URL" | grep -q -F -- "$TEST_CONTENT"; then
  echo 'Lookup server health check failed!'
  exit 1
fi

# Start nginx in the background. As soon as nginx starts listening on port 80,
# Google Cloud Run will start serving incoming traffic from this instance.
nginx

# Keep running as long as lookup server does.
wait $LOOKUP_SERVER_PID
