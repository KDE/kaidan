-- SPDX-FileCopyrightText: 2026 Linus Jahn <lnj@kaidan.im>
--
-- SPDX-License-Identifier: CC0-1.0

modules_enabled = {
    "roster"; "saslauth"; "tls"; "disco"; "carbons"; "pep"; "private"; "blocklist";
    "vcard4"; "vcard_legacy"; "bookmarks"; "version"; "time"; "ping"; "mam"; "smacks"; "posix";
}
modules_disabled = { "s2s" }
c2s_require_encryption = true
allow_unencrypted_plain_auth = true
authentication = "internal_plain"
storage = "internal"
log = { info = "*console" }
daemonize = false
certificates = "/etc/prosody/certs"

VirtualHost "localhost"
