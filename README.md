# SphereServer
Game server for UO

[![Windows build status](https://ci.appveyor.com/api/projects/status/befpuqebq01caopi?svg=true)](https://ci.appveyor.com/project/coruja747/source)
[![Linux build status](https://travis-ci.org/Sphereserver/Source.svg?branch=master)](https://travis-ci.org/Sphereserver/Source)
[![Join the chat at https://gitter.im/Sphereserver/Source](https://badges.gitter.im/Join%20Chat.svg)](https://gitter.im/Sphereserver/Source)

## Project details
[Automatic builds](http://nightly.prerelease.sphere.torfo.org/)

## Building
Project files can be build with CMake for both Linux (GCC) and Windows (MSVC and MinGW), old project files are still present in the source for now.

### Windows
#### Visual Studio
* Open 'GraySvr.vcxproj' file using Visual Studio (if you're using VS > 2010 it will request an update on project file, just click OK to update)
* On top menu, select the build configuration (Debug/Local/Nightly/Release) and click on 'Build > Build Solution' to compile

#### CMake
You can compile using provided CMake files.

### Linux
* Open the terminal on sphere source dir and use the command "makefile NIGHTLY=1 -f makefile" to compile

MySQL libs will be required if not installed yet. You can install using these commands:

#### Ubuntu
* sudo apt-get install libmysqld-dev
* sudo apt-get install libmysql++ libmysql++-dev
* sudo apt-get install libmysqlclient-dev:i386

#### CentOS
* yum install mysql
* yum install mysql-libs
* yum install mysql-devel

## Coding notes for new contributors
* Make sure you can compile and run the program before pushing a commit.
* Rebasing instead of pulling the project is a better practice to avoid unnecessary "merge branch master" commits.
* Removing/Changing/Adding anything that was working in one way for years should be followed by an ini setting if the changes cannot be replicated from script to keep some backwards compatibility.

## Licensing
Copyright 2016 SphereServer development team

Licensed under the Apache License, Version 2.0 (the "License").<br>
You may not use any file of this project except in compliance with the License.<br>
You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
