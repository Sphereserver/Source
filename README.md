# SphereServer

Ultima Online game server, developed in C++

## Download

- [GitHub Releases](../../releases)
- [SphereServer Forum](https://forum.spherecommunity.net/sshare.php?srt=4)

> [!NOTE]
> Recommended OS for pre-compiled builds:
> - Windows: Windows 10 or later, Windows Server 2016 or later
> - Linux: Debian 12 or later, Ubuntu 22.04 or later (requires MySQL client library installed)

> [!IMPORTANT]
> To ensure file integrity, only download builds from official sources and verify the SHA-256 checksum using the system terminal:
> - Windows: `certutil -hashfile "path/to/file" SHA256`
> - Linux: `sha256sum "path/to/file"`

## Building on Windows (Visual Studio)

### System requirements

- OS: Windows 10 or later, Windows Server 2016 or later
- IDE: Visual Studio 2022 or later (VS Code is not supported)

### Set up environment

Open `Visual Studio Installer`, click on `Modify`, and install the following components:
- Workload: `Desktop Development with C++`
- Individual component: `MSVC v143 - VS 2022 C++ x64/x86 build tools`

> [!NOTE]
> The project uses `MSVC v143` platform toolset from VS 2022 for backward compatibility. When opening the project file for the first time using a newer VS, it will request to retarget the project platform toolset to the latest version already installed (e.g., `MSVC v145` from VS 2026). You can either choose to retarget it, or install the `MSVC v143` individual component and safely ignore the request

### Build the project

- Open the project file `SphereSvr.vcxproj` using Visual Studio
- On the top menu, select the build configuration (`Debug`/`Nightly`/`Release`) and click on `Build > Build Solution`

## Building on Linux (GCC)

### Set up environment

#### Debian-based (Debian 11 or later, Ubuntu 20.04 or later)

1. Add MySQL 8.0 support to the package repository (only required for Debian 11 or later, Ubuntu 26.04 or later):
   ```
   curl -fL https://dev.mysql.com/get/mysql-apt-config_0.8.39-1_all.deb -o mysql-apt-config.deb
   echo "mysql-apt-config mysql-apt-config/select-server select mysql-8.0" | sudo debconf-set-selections
   sudo DEBIAN_FRONTEND=noninteractive dpkg -i mysql-apt-config.deb
   sudo apt-get update
   rm mysql-apt-config.deb
   ```

2. Install required packages:
   ```
   sudo apt-get install git gcc g++ make libmysqlclient-dev=8.0*
   ```

#### RHEL-based (RHEL, Rocky Linux, AlmaLinux, Oracle Linux) 8 or later

1. Enable MySQL 8.0 module stream:
   ```
   sudo dnf module enable mysql:8.0
   ```

2. Install required packages:
   ```
   sudo dnf install git gcc-c++ glibc-devel mysql-devel
   ```

### Get the source code

```
git clone https://github.com/Sphereserver/Source.git
cd Source
```

### Build the project

| Build type | Command          |
| ---------- | ---------------- |
| Debug      | `make DEBUG=1`   |
| Nightly    | `make NIGHTLY=1` |
| Release    | `make`           |

## Contributing

- Make sure you can compile and run the program before pushing a commit.
- Rebasing instead of pulling the project is a better practice to avoid unnecessary "merge branch master" commits.
- To keep backward compatibility, adding/removing/changing anything that was working in one way for years should be followed by an .ini setting when the changes can't be replicated on scripts.
- Prefer using C++ named casts (`static_cast<type>(expression)`, `dynamic_cast<type>(expression)`, `const_cast<type>(expression)`, `reinterpret_cast<type>(expression)`) over C-style casts (`(type)expression`) to ensure invalid casts will cause a compiler error to get fixed at compile time, preventing the compiler from applying an unsafe cast that may cause runtime errors.

## License

Copyright 2026 SphereServer development team

Licensed under the Apache License, Version 2.0 (the "License").  
You may not use any file of this project except in compliance with the License.  
You may obtain a copy of the License at <https://www.apache.org/licenses/LICENSE-2.0>
