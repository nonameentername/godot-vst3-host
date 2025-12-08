godot-vst3-host
===============

[![builds](https://github.com/nonameentername/godot-vst3-host/actions/workflows/builds.yml/badge.svg)](https://github.com/nonameentername/godot-vst3-host/actions/workflows/builds.yml)
[![docker_builds](https://github.com/nonameentername/godot-vst3-host/actions/workflows/build_images.yml/badge.svg)](https://github.com/nonameentername/godot-vst3-host/actions/workflows/build_images.yml)

Godot GDExtension library to allow using VST3 plugins in Godot.  Currently works with Godot v4.5 stable release.

Projects using godot-vst3-host
------------------------------

* [godot-vst3-host-example](https://github.com/nonameentername/godot-vst3-host-example) - Simple godot-vst3-host example.

How to Install
--------------
Download latest [release](https://github.com/nonameentername/godot-vst3-host/releases/latest) and extract into Godot project.

How to Build
------------

This project uses docker to build the project for different platforms.
The build scripts were developed using Ubuntu (x86_64).


## Instructions

1. Install docker using apt:

```bash
sudo apt install docker.io
```

2. Add user to docker group:

```bash
sudo usermod -a -G docker $USER
```

3. Initialize git submodules:

```bash
git submodule update --init --recursive
```

4. Build project using make:

```bash
#build all platforms
make all

#build for ubuntu
make ubuntu

#build for windows
make mingw

#build for MacOS
make osxcross
```
