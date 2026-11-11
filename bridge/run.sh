#!/bin/bash
# Wrapper para arrancar el bridge asegurando el PATH de Node (NVM, Homebrew, etc.)
export PATH="$HOME/.nvm/versions/node/$(ls $HOME/.nvm/versions/node 2>/dev/null | tail -n 1)/bin:/opt/homebrew/bin:/usr/local/bin:$PATH"

NODE_BIN=$(which node)

if [ -z "$NODE_BIN" ]; then
    echo "[Error] No se encontró node en PATH" >&2
    exit 1
fi

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
exec "$NODE_BIN" "$DIR/herdr-bridge.mjs"
