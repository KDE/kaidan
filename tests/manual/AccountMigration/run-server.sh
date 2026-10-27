#!/bin/sh
# SPDX-FileCopyrightText: 2026 Linus Jahn <lnj@kaidan.im>
#
# SPDX-License-Identifier: CC0-1.0

# Starts a fresh Prosody container on localhost:5222 with the accounts used by the
# AccountMigration manual test. Running it again resets all server data.

set -e

IMAGE=docker.io/prosodyim/prosody:13.0
CONTAINER=kaidan-account-migration-prosody
SOURCE_DIR=$(cd "$(dirname "$0")" && pwd)
CERT_DIR=${XDG_RUNTIME_DIR:-/tmp}/$CONTAINER-certs
PODMAN=$(command -v podman || command -v docker)

if [ ! -f "$CERT_DIR/localhost.crt" ]; then
    mkdir -p "$CERT_DIR"
    openssl req -x509 -newkey rsa:2048 -nodes -days 365 -subj /CN=localhost \
        -addext subjectAltName=DNS:localhost \
        -keyout "$CERT_DIR/localhost.key" -out "$CERT_DIR/localhost.crt" 2>/dev/null
    chmod 644 "$CERT_DIR"/*
fi

"$PODMAN" rm -f "$CONTAINER" >/dev/null 2>&1 || true
"$PODMAN" run -d --name "$CONTAINER" -p 127.0.0.1:5222:5222 \
    -v "$SOURCE_DIR/prosody.cfg.lua:/etc/prosody/prosody.cfg.lua:ro,Z" \
    -v "$CERT_DIR:/etc/prosody/certs:ro,Z" \
    "$IMAGE" >/dev/null
sleep 3

for user in old new alice bob; do
    "$PODMAN" exec "$CONTAINER" prosodyctl register "$user" localhost secret >/dev/null
done

echo "Server running. Run the test with:"
echo "  AccountMigration $CERT_DIR/localhost.crt"
