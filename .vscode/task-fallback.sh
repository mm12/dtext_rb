#!/bin/sh
set -e

# Usage: task-fallback.sh "docker-compose ..." "local fallback..."
# If docker-compose is available, run the first argument as a shell command;
# otherwise run the second argument.

if command -v docker-compose >/dev/null 2>&1; then
  eval "$1"
else
  echo "docker-compose not found; running local fallback: $2"
  eval "$2"
fi
