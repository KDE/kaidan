<!--
SPDX-FileCopyrightText: 2026 Linus Jahn <lnj@kaidan.im>

SPDX-License-Identifier: CC0-1.0
-->

# Account Migration Test

Manual end-to-end test of the account migration against a local XMPP server.
It runs Kaidan's core without UI and performs the same steps as the UI.

## What is tested

1. Log into `old@localhost`.
2. Add the contacts `alice@localhost` (name "Alice", group "Friends") and `bob@localhost` (name "Bob")
   to the old account's roster.
3. Change local contact settings: pinning position of Alice to 4 and chat state sending of Bob to
   disabled.
4. Start the migration (`AccountController::startMigration()`), which exports the old account's
   data.
5. Log into `new@localhost` like the start page does, which finalizes the migration (importing the
   data into the new account, publishing the moved statement and notifying the contacts).
6. Check that:
   * the migration finishes,
   * Alice and Bob are in the new account's roster,
   * Alice's name and group are migrated (they are only part of the exported roster, not of the
     subscription requests sent to the contacts),
   * the local contact settings are migrated.

The test prints each step and ends with `RESULT: SUCCESS` (exit code 0) or `RESULT: FAILURE`
(exit code 1).
It uses a temporary directory for its database and settings, so your own Kaidan data is not
touched.

## Requirements

* `podman` or `docker`
* `openssl`
* Kaidan built with `BUILD_TESTING` enabled

## Usage

Start the server:

```sh
tests/manual/AccountMigration/run-server.sh
```

It starts Prosody in the container `kaidan-account-migration-prosody` on `localhost:5222`, creates a
self-signed certificate and registers the accounts `old`, `new`, `alice` and `bob` with the password
`secret`.
Afterwards, it prints the command for running the test, e.g.:

```sh
build/bin/AccountMigration /run/user/1000/kaidan-account-migration-prosody-certs/localhost.crt
```

The certificate is passed to the test so that it is trusted for the TLS connection.

The test changes the server's data.
Run `run-server.sh` again before each test run to start with a fresh server.

Stop the server afterwards:

```sh
podman rm -f kaidan-account-migration-prosody
```

## Debugging

Enable logging of the XMPP traffic:

```sh
QT_LOGGING_RULES="im.kaidan.xmpp.debug=true" build/bin/AccountMigration <certificate>
```
