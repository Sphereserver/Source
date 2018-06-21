# SphereServer
Game server for Ultima Online

[![Build status](https://ci.appveyor.com/api/projects/status/befpuqebq01caopi?svg=true)](https://ci.appveyor.com/project/coruja747/source)
[![Coverity Scan Build Status](https://scan.coverity.com/projects/16062/badge.svg)](https://scan.coverity.com/projects/sphere-server)
[![Join the chat at https://gitter.im/SphereServer/Source](https://badges.gitter.im/Join%20Chat.svg)](https://gitter.im/SphereServer/Source)

## Downloads

[Automatic builds](https://forum.spherecommunity.net/sshare.php?srt=4)

## Building
Project files can be build on Windows (VS, CMake, MinGW) and Linux (GCC, CMake).

Even using 64bit OS, Sphere must be compiled/executed in 32bit mode to work properly. Project files included on source code will compile it in 32bit mode by default.

### Windows
#### Visual Studio (VS)
* Open the project file `SphereSvr.vcxproj` using Visual Studio
* On top menu, select the build configuration (Debug/Local/Nightly/Release) and click on `Build > Build Solution` to compile

##### NOTES:
* **VS > 2010 users:** When opening `SphereSvr.vcxproj` for the first time it will request an update on project file, just click OK to update
* **VS 2017 users:** VS 2017 have an modular installation, it comes with just basic components and extra components must be installed as needed. To use Sphere source you must open **Visual Studio Installer** to install `Desktop Development with C++` workload and `Windows Universal CRT SDK` individual component

#### CMake / MinGW
You can compile using provided files.

### Linux
Some libs are required to compile/execute Sphere on Linux. Use these commands to install:

#### Ubuntu 12.x ~ 18.x
If you're using 64bit OS, enable 32bit support using:
```
sudo dpkg --add-architecture i386
sudo apt-get update
sudo apt-get dist-upgrade
```
If you're using Ubuntu 14.x (or older), the default package repository only have support up to MySQL 5.6, so add support for MySQL 5.7 packages using:
```
sudo add-apt-repository 'deb http://repo.mysql.com/apt/ubuntu/ trusty mysql-5.7'
sudo apt-get update
```
Install required packages:
```
sudo apt-get install git
sudo apt-get install gcc-multilib
sudo apt-get install g++-multilib
sudo apt-get install make:i386
sudo apt-get install libmysql++-dev:i386
```

#### CentOS 6 / 7 - Red Hat 6 / 7
If you're using CentOS 6 / Red Hat 6, the default package repository only have support up to MySQL 5.6, so add support for MySQL 5.7 packages using:
```
sudo rpm -Uvh https://dev.mysql.com/get/mysql57-community-release-el6-11.noarch.rpm
```
If you're using CentOS 7 / Red Hat 7, the default package repository only have support to MariaDB instead MySQL, so add support for MySQL 5.7 packages using:
```
sudo rpm -Uvh https://dev.mysql.com/get/mysql57-community-release-el7-11.noarch.rpm
```
Install required packages:
```
sudo yum install git
sudo yum install gcc-c++
sudo yum install glibc-devel.i686
sudo yum install mysql-community-devel.i686 mysql-community-libs.i686
```

#### Get the source code and compile it
```
git clone https://github.com/SphereServer/Source.git
cd Source
make NIGHTLY=1 -f makefile
```
**NOTE:** Sphere source is also compatible with CMake compiler for users that prefer it over GCC

## Coding notes for new contributors
* Make sure you can compile and run the program before pushing a commit.
* Rebasing instead of pulling the project is a better practice to avoid unnecessary "merge branch master" commits.
* Removing/Changing/Adding anything that was working in one way for years should be followed by an ini setting if the changes cannot be replicated from script to keep some backwards compatibility.

## Licensing
Copyright 2018 SphereServer development team

Licensed under the Apache License, Version 2.0 (the "License").<br>
You may not use any file of this project except in compliance with the License.<br>
You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
