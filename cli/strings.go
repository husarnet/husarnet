// Copyright (c) 2025 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
package main

const runSelfWithSudoQuestion string = "This operation requires superuser access. Rerun with sudo?"

const defaultsIniTemplate string = `; consult the documentation before editing. See https://husarnet.com/docs/defaults-ini

; Husarnet INI file, which is read both by the Daemon and the CLI on startup
; this is convenient way of providing a default value for an environment variable used by Husarnet
; the values that are provided here are the defaults, which means env vars set directly on the running process
; (for example via /etc/default/husarnet on Linux) will override them.

[common]
; instance_fqdn = prod.husarnet.com
; log_verbosity = info
; enable_hooks = false

[daemon]
; daemon_interface = hnet0

[cli]
; daemon_api_secret = some-secret`
