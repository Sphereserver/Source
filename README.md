# SphereServer

Ultima Online game server, developed in C++

## Download

* [GitHub Releases](../../releases)
* [SphereServer website](https://forum.spherecommunity.net/sshare.php?srt=4)

> [!IMPORTANT]
> For maximum security, only download builds from official sources and verify the file SHA-256 checksum using the system terminal:
> * Windows (PowerShell): `Get-FileHash "path/to/file"`
> * Linux: `sha256sum "path/to/file"`

## Building on Windows (Visual Studio)

* Open the project file `SphereSvr.vcxproj` using Visual Studio
* On top menu, select the build configuration (Debug/Nightly/Release) and click on `Build > Build Solution` to compile

### Requirements:

* Windows 10 or later / Windows Server 2016 or later
* Visual Studio 2022 or later (VS Code is not supported)
* Open `Visual Studio Installer` menu, click on "Modify" and install these components:
  * Workload: `Desktop Development with C++`
  * Individual component: `MSVC v143 - VS 2022 C++ x64/x86 build tools`

> [!NOTE]
> The project uses `MSVC v143` from VS 2022 for backward compatibility. When using VS newer than 2022, you can either choose to update the project on `Project > Retarget solution` to use the latest version already installed (e.g., `MSVC v145` from VS 2026), or install the `MSVC v143` individual component and safely ignore the message requesting an update to the latest version

## Building on Linux (GCC)

### Set up environment

<details>

  <summary>Ubuntu 14.04 or later (click to expand)</summary>

  #### Add MySQL 5.7 support

  * Ubuntu 14.04:
    * Add MySQL 5.7 support to the package repository
      ```
      sudo gpg --homedir /tmp --no-default-keyring --keyring /usr/share/keyrings/mysql.gpg --keyserver keyserver.ubuntu.com --recv-keys B7B3B788A8D3785C
      echo "deb [signed-by=/usr/share/keyrings/mysql.gpg] https://repo.mysql.com/apt/ubuntu/ bionic mysql-5.7" | sudo tee /etc/apt/sources.list.d/mysql.list > /dev/null
      sudo apt-get update
      ```
  * Ubuntu 16.04 to 18.04:
    * > Skip this step (MySQL 5.7 is already supported)
  * Ubuntu 20.04 or later:
    * Remove default MySQL package
      ```
      sudo apt-get -q remove libmysqlclient-dev
      ```
    * Add MySQL 5.7 support to the package repository
      ```
      sudo gpg --homedir /tmp --no-default-keyring --keyring /usr/share/keyrings/mysql.gpg --keyserver keyserver.ubuntu.com --recv-keys B7B3B788A8D3785C
      echo "deb [signed-by=/usr/share/keyrings/mysql.gpg] https://repo.mysql.com/apt/ubuntu/ bionic mysql-5.7" | sudo tee /etc/apt/sources.list.d/mysql.list > /dev/null
      sudo apt-get update
      ```

  #### Install required packages

  ```
  sudo apt-get install git gcc g++ make libmysqlclient-dev=5.7*
  ```

</details>

<details>

  <summary>RHEL 6 / Oracle Linux 6 / CentOS 6 or later (click to expand)</summary>

  #### Add MySQL support

  * Add MySQL support to the package repository
    * RHEL 6 / Oracle Linux 6 / CentOS 6:
      ```
      sudo yum localinstall https://dev.mysql.com/get/mysql80-community-release-el6-11.noarch.rpm
      ```
    * RHEL 7 / Oracle Linux 7 / CentOS 7:
      ```
      sudo yum localinstall https://dev.mysql.com/get/mysql84-community-release-el7-4.noarch.rpm
      ```
    * RHEL 8 / Oracle Linux 8 / CentOS 8:
      ```
      sudo yum localinstall https://dev.mysql.com/get/mysql84-community-release-el8-3.noarch.rpm
      sudo yum module disable mysql
      ```
    * RHEL 9 / Oracle Linux 9 / CentOS 9:
      ```
      sudo yum localinstall https://dev.mysql.com/get/mysql84-community-release-el9-4.noarch.rpm
      sudo yum module disable mysql
      ```
    * RHEL 10 / Oracle Linux 10 / CentOS 10:
      ```
      sudo yum localinstall https://dev.mysql.com/get/mysql84-community-release-el10-3.noarch.rpm
      sudo yum module disable mysql
      ```

  * Enable MySQL 5.7 in the package repository
    ```
    sudo yum-config-manager --disable mysql-8.4-lts-community
    sudo yum-config-manager --disable mysql80-community
    sudo yum-config-manager --enable mysql57-community
    ```

  #### Install required packages

  ```
  sudo yum install git gcc-c++ glibc-devel mysql-community-devel
  ```

</details>

### Get the source code

```
git clone https://github.com/SphereServer/Source.git
cd Source
```

### Compile the source code

* To compile Debug build:
  ```
  make DEBUG=1
  ```
* To compile Nightly build:
  ```
  make NIGHTLY=1
  ```
* To compile Release build:
  ```
  make
  ```

## Coding notes for new contributors

* Make sure you can compile and run the program before pushing a commit.
* Rebasing instead of pulling the project is a better practice to avoid unnecessary "merge branch master" commits.
* To keep backward compatibility, adding/removing/changing anything that was working in one way for years should be followed by an .ini setting when the changes can't be replicated on scripts.
* Prefer using C++ named casts (`static_cast<type>(expression)`, `dynamic_cast<type>(expression)`, `const_cast<type>(expression)`, `reinterpret_cast<type>(expression)`) over C-style casts (`(type)expression`) to make sure that invalid casts will throw a compiler error to get fixed at compile-time, preventing the compiler from silently applying an unsafe cast that may throw a runtime error.

## License

Copyright 2026 SphereServer development team

Licensed under the Apache License, Version 2.0 (the "License").  
You may not use any file of this project except in compliance with the License.  
You may obtain a copy of the License at <https://www.apache.org/licenses/LICENSE-2.0>
