# Sphereserver

## Join gitter chat
[![Join the chat at https://gitter.im/Sphereserver/Source](https://badges.gitter.im/Join%20Chat.svg)](https://gitter.im/Sphereserver/Source)

## Project details

[Automatic builds](http://nightly.prerelease.sphere.torfo.org/)

## Building

### Ubuntu

Install mysql library
* sudo apt-get install libmysqld-dev
* sudo apt-get install libmysql++ libmysql++-dev
* sudo apt-get install libmysqlclient-dev:i386

### Windows

Create a .sln file
* Open src\graysvr\GraySvr.vcxproj with Visual Studio, press crtl + s to save the solution and choose a name for the .sln file, use this file from now.

## Coding Notes (add as you wish to standardize the coding for new contributors)

* Make sure you can compile and run the program before pushing a commit.
* Rebasing instead of pulling the project is a better practice to avoid unnecessary "merge branch master" commits.

## Licensing
