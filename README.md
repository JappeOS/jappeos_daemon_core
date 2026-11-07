<h1 align="center">
  <img src="https://raw.githubusercontent.com/JappeOS/JappeOS/dev/Icons/jappeos-logo-banner-white-512.png" width="120"><br>
  jappeos_daemon_core
</h1>

<p align="center">
  <strong>The core daemon library for JappeOS, used by system and session services.</strong>
</p>

<p align="center">
  <a href="./issues"><img src="https://img.shields.io/github/issues/JappeOS/jappeos_daemon_core?style=plastic&color=edda09"></a>
  <a href="./pulls"><img src="https://img.shields.io/github/issues-pr/JappeOS/jappeos_daemon_core?style=plastic&color=40a842"></a>
  <a href="./LICENSE"><img src="https://img.shields.io/github/license/JappeOS/jappeos_daemon_core?style=plastic&color=9d09ed"></a>
  <img src="https://img.shields.io/badge/arch-x86__64-blue?style=plastic">
  <img src="https://img.shields.io/badge/status-experimental-orange?style=plastic">
  <a href="https://discord.gg/dRtU4HR"><img src="https://img.shields.io/discord/716673375946407972?style=plastic&color=3250a8"></a>
</p>

---

## Overview

This is the core daemon library for JappeOS.
It's a library intended to be used by system or session daemons and services on JappeOS.

## Features

* Utilities, such as command runner, OS utils, exceptions, scope-guard etc.
* Logging that supports the systemd journal via stdout.
* A clean DBus wrapper library.
* Daemon lifecycle.

## Role in the OS

Allows easy creation of daemons and services on session and system level.

## Depend on it

To use this library in your daemon, add the following to your `CMakeLists.txt`:
```
include(FetchContent)
FetchContent_Declare(
        jappeos_daemon_core
        GIT_REPOSITORY https://github.com/JappeOS/jappeos_daemon_core.git
        GIT_TAG <latest tag>
)
FetchContent_MakeAvailable(jappeos_daemon_core)
```

Replace `<latest tag>` with the latest version of this library.

## Notes

- This project is **Linux-only** and relies on Linux-specific APIs and system services.

## Contributing

Contributions of all kinds are welcome and appreciated. You can help the project by:

- ⭐ Starring the repository to show your support
- 💖 Sponsoring the project (if available)
- 🐞 Reporting bugs via [GitHub Issues](./issues)
- 💡 Requesting or discussing new features

For code contributions, please see [`CONTRIBUTING.md`](./CONTRIBUTING.md) for guidelines.

## License

This repository is part of the JappeOS project and is licensed under the terms described in the [`LICENSE`](./LICENSE) file.
