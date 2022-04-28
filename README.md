# SphereServer
Game server for Ultima Online

[![Coverity Scan Build Status](https://scan.coverity.com/projects/16074/badge.svg)](https://scan.coverity.com/projects/sphereserver-source)
[![Gitter](https://badges.gitter.im/Sphereserver/Source.svg)](https://gitter.im/Sphereserver/Source)

## Download
[Automatic builds](https://forum.spherecommunity.net/sshare.php?srt=4)

## Building
Project can be compiled on Windows (Visual Studio) and Linux (GCC)

### Windows (Visual Studio)
* Open the project file `SphereSvr.vcxproj` using Visual Studio
* On top menu, select the build configuration (Debug/Local/Nightly/Release), platform (x86/x64), and click on `Build > Build Solution` to compile

#### NOTES:
* Required version: VS2015 or later (VS Code is not supported)
* When using VS >= 2017:
  * When opening `SphereSvr.vcxproj` for the first time it will request an update on project file, just click OK to update (if update request doesn't get opened automatically, just open it on `Project > Retarget solution`)
  * Newest VS have an modular installation which comes with just basic components, and extra components must be installed as needed. To compile Sphere you must open `Visual Studio Installer` to install `Desktop Development with C++` workload

### Linux (Ubuntu)
#### Add architecture support
* To compile 32bit build on 32bit OS or 64bit build on 64bit OS:
  ```
  # Skip this step (architecture is already supported by OS)
  ```
* To compile 32bit build on 64bit OS:
  ```
  sudo dpkg --add-architecture i386
  sudo apt-get update
  sudo apt-get dist-upgrade
  ```

#### Install MySQL 5.7 client
* Ubuntu 14.10 or older:
  * Default package repository only have support up to MySQL 5.6, so add MySQL 5.7 support
    ```
    sudo add-apt-repository 'deb http://repo.mysql.com/apt/ubuntu/ precise mysql-5.7'
    sudo apt-get update
    ```
  * Install MySQL
    * To compile 32bit build on 32bit OS or 64bit build on 64bit OS:
      ```
      sudo apt-get install libmysqlclient-dev
      ```
    * To compile 32bit build on 64bit OS:
      ```
      sudo apt-get install libmysqlclient-dev:i386
      ```

* Ubuntu 15.04 to 19.04:
  * Install MySQL
    * To compile 32bit build on 32bit OS or 64bit build on 64bit OS:
      ```
      sudo apt-get install libmysqlclient-dev
      ```
    * To compile 32bit build on 64bit OS:
      ```
      sudo apt-get install libmysqlclient-dev:i386
      ```

* Ubuntu 19.10 or later:
  * Default package repository dropped support for MySQL 5.7, so add it back
    ```
    sudo add-apt-repository 'deb http://repo.mysql.com/apt/ubuntu/ bionic mysql-5.7'
    sudo apt-get update --allow-insecure-repositories
    ```
  * Install MySQL
    * To compile 32bit build on 32bit OS or 64bit build on 64bit OS:
      ```
      sudo apt-get install libmysqlclient-dev=5.7*
      ```
    * To compile 32bit build on 64bit OS:
      ```
      sudo apt-get install libmysqlclient-dev:i386=5.7*
      ```

#### Install required packages
* To compile 32bit build on 32bit OS or 64bit build on 64bit OS:
  ```
  sudo apt-get install git
  sudo apt-get install gcc
  sudo apt-get install g++
  sudo apt-get install make
  ```
* To compile 32bit build on 64bit OS:
  ```
  sudo apt-get install git
  sudo apt-get install gcc-multilib
  sudo apt-get install g++-multilib
  sudo apt-get install make
  ```

### Linux (CentOS / Red Hat)
#### Add MySQL 5.7 support
* Add MySQL support on package repository
  * CentOS 6 / Red Hat 6:
    ```
    sudo yum localinstall https://dev.mysql.com/get/mysql80-community-release-el6-6.noarch.rpm
    ```
  * CentOS 7 / Red Hat 7:
    ```
    sudo yum localinstall https://dev.mysql.com/get/mysql80-community-release-el7-6.noarch.rpm
    ```
  * CentOS 8 / Red Hat 8:
    ```
    sudo yum localinstall https://dev.mysql.com/get/mysql80-community-release-el8-4.noarch.rpm
    ```

* Configure MySQL version on package repository
  ```
  sudo yum-config-manager --disable mysql80-community
  sudo yum-config-manager --enable mysql57-community
  ```

#### Install required packages
* To compile 32bit build on 32bit OS or 64bit build on 64bit OS:
  ```
  sudo yum install git
  sudo yum install gcc-c++
  sudo yum install glibc-devel
  sudo yum install mysql-community-devel mysql-community-libs
  ```
* To compile 32bit build on 64bit OS:
  ```
  sudo yum install git
  sudo yum install gcc-c++
  sudo yum install glibc-devel.i686
  sudo yum install mysql-community-devel.i686 mysql-community-libs.i686
  ```

### Get the source code
```
git clone https://github.com/SphereServer/Source.git
cd Source
```

### Compile the source code
* To compile 32bit build:
```
make NIGHTLY=1 x86=1
```
* To compile 64bit build:
```
make NIGHTLY=1
```

## Coding notes for new contributors
* Make sure you can compile and run the program before pushing a commit.
* Rebasing instead of pulling the project is a better practice to avoid unnecessary "merge branch master" commits.
* Adding/removing/changing anything that was working in one way for years should be followed by an ini setting when the changes can't be replicated on scripts to keep backwards compatibility.

## Licensing
Copyright 2022 SphereServer development team

Licensed under the Apache License, Version 2.0 (the "License").<br>
You may not use any file of this project except in compliance with the License.<br>
You may obtain a copy of the License at <https://www.apache.org/licenses/LICENSE-2.0>
