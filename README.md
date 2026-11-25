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
```cmake
include(FetchContent)
FetchContent_Declare(
        jappeos_daemon_core
        GIT_REPOSITORY https://github.com/JappeOS/jappeos_daemon_core.git
        GIT_TAG <latest tag>
)
FetchContent_MakeAvailable(jappeos_daemon_core)
```

Replace `<latest tag>` with the latest version of this library.

## Create your daemon

### Getting started with the basics

First, create your daemon as a regular executable C++ project. Other programming languages can be used, as long as you have bindings for it.

Then, add this project as a dependency to your daemon, then create a main method in your `src/main.cpp`:
```c++
#include "daemon.h"
#include "my_service.h"

using namespace JappeStudios::JappeOS::DaemonCore;

CREATE_DAEMON(
    [] { return new JappeStudios::JappeOS::MyServiced::MyService(); },
    DaemonDBusConfig::System("org.jappeos.MyServiced1")
);
```
DBus is commonly used for IPC (Inter Process Communication) in JappeOS, and many other Linux-based operating systems.

The first parameter of `CREATE_DAEMON` tells the library how your daemon class should be created. It should usually be
a call to a parameterless constructor, where initialization work for the daemon is left to the constructor and methods
called by it.

The second parameter of `CREATE_DAEMON` tells the library how to configure DBus to enable IPC. Leave it `std::nullopt`
if you wish to not use IPC.

The `DaemonDBusConfig` object defines how the connection is created, and under what bus name it is registered, and optionally, DBus name acquisition flags (default is `DBUS_NAME_FLAG_DO_NOT_QUEUE`). You can learn more about DBus here: https://dbus.freedesktop.org/doc/dbus-specification.html

This example service runs under the name "org.jappeos.MyServiced1" under the system bus.

Now, we should create a simple daemon inside your `my_service.h` header (included at the top of your `src/main.cpp` file):
```c++
#pragma once
#include "daemon.h"
#include "logging/logger.h"
using namespace JappeStudios::JappeOS::DaemonCore::DBus;

namespace JappeStudios::JappeOS::MyServiced
{
    class MyService : public DaemonCore::DaemonClass
    {
    public:
        explicit MyService();
        ~MyService();

    private:
        Connection& _conn;
        Object      _object;
        Interface&  _iface;
        
    private:
        CREATE_LOGGER("MyService")
    }
}
```

Then also create the `my_service.cpp` source file:
```c++
#include "my_service.h"

namespace JappeStudios::JappeOS::MyServiced
{
    MyService::MyService() :
        _conn(DaemonCore::Daemon::GetConnection()),
        _object(_conn, DaemonCore::Daemon::GetBaseObject()),
        _iface(_object.CreateInterface(DaemonCore::Daemon::GetBaseInterface()))
    {
        Log().Debug("Daemon initialized!");
    }
    
    MyService::~MyService()
    {
        Log().Debug("Daemon shut down!");
    }
}
```

Great! Now you have a simple daemon that logs "Daemon initialized!" on startup, and "Daemon shut down!" upon shutdown.
The library handles lifecycle internals for you.

The class now gets a reference to the DBus connection, creates an interface based on the bus-name you provided earlier, and creates and object from the interface. The resources get automatically cleaned up on shutdown as expected.

The constructor should be used to allocate methods, properties, signals, listeners etc., and prepare the service.
If the service is DBus activatable via a systemd service (more on that later), the service reports that it is ready **after** the constructor has run.
This basically means that the name acquisition happens after your daemon class has been constructed.

> [!IMPORTANT]
> While you can do somewhat computationally heavy work in the constructor, do note that doing long-lasting or computationally-heavy work on the main thread is not recommended. Your constructor, destructor and DBus methods get called on the main thread. If you need to do heavy work, consider using a separate background thread, and joining it in the destructor safely. By offloading/deferring heavy work to a background thread, you avoid making the daemon unresponsive.

### Creating a systemd daemon

To create a systemd daemon, you don't actually need to rewrite everything, instead, you create a systemd `jappeos-myserviced.service` file for it:
```ini
[Unit]
Description=My Service
After=dbus.service systemd-logind.service graphical-pre.target
Requires=dbus.service
Wants=graphical-pre.target

[Service]
Type=simple
ExecStart=/usr/lib/jappeos_myserviced/jappeos_myserviced
Restart=no
StandardOutput=journal
StandardError=journal

[Install]
WantedBy=basic.target
```

> [!TIP]
> This readme is still a work in progress. Expect it to not be finished yet.

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
