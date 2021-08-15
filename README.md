<p align="center">
  <img src="res/textures/logo.png" width="20%">
</p>
<p align="center">
  <a href="https://github.com/Paul-Hi/Mango/actions?query=workflow%3A%22Continuous+Integration%22"><img alt="Continuous Integration Status" src="https://github.com/Paul-Hi/Mango/workflows/Continuous%20Integration/badge.svg"></a>
  <br>
  <br>
  <a href="https://github.com/Paul-Hi/Mango"><img alt="Tag" src="https://img.shields.io/github/tag/Paul-Hi/Mango.svg"></a>
  <a href="https://paul-hi.github.io/Mango/"><img alt="Website" src="https://img.shields.io/website-up-down-green-red/http/shields.io.svg"></a>
  <a href="https://github.com/Paul-Hi/Mango/blob/master/LICENSE"><img alt="License" src="https://badgen.net/github/license/Paul-Hi/Mango"></a>
  <a href="https://img.shields.io/github/languages/code-size/Paul-Hi/Mango.svg"><img alt="Code Size" src="https://img.shields.io/github/languages/code-size/Paul-Hi/Mango.svg"></a>
  <a href="https://www.doxygen.nl/index.html"><img alt="Doxygen" src="https://img.shields.io/badge/document-doxygen-brightgreen"></a>
  <br>
  <a href="https://github.com/Paul-Hi/Mango/graphs/commit-activity"><img alt="Commits" src="https://badgen.net/github/commits/Paul-Hi/Mango"></a>
  <a href="https://github.com/Paul-Hi/Mango/graphs/commit-activity"><img alt="Last Commit" src="https://badgen.net/github/last-commit/Paul-Hi/Mango"></a>
  <a href="https://github.com/Paul-Hi/Mango/issues/"><img alt="Issues" src="https://img.shields.io/github/issues/Paul-Hi/Mango.svg"></a>
  <a href="https://github.com/Paul-Hi/Mango/pull/"><img alt="Pull Requests" src="https://img.shields.io/github/issues-pr/Paul-Hi/Mango.svg"></a>
  <a href="https://github.com/Paul-Hi/Mango/graphs/contributors"><img alt="Contributors" src="https://img.shields.io/github/contributors/Paul-Hi/Mango.svg"></a>
</p>

# Mango

Mango is an Open Source Framework. Or at least it should be in the future.
A playground for people like me.

Just to be clear: This is not the first attempt!
I plan to implement various features, with main focus on computer graphics.
A few of them are described below, but there will be added a lot more in the future.
There should also always be a documentation and tests to ensure cleaner and more functional code.

If you have suggestions for improvement or find a bug just let me know.
Keep in mind that I do this in my spare time, besides studying and working.
Also have a look at the [Website](https://paul-hi.github.io/Mango/)!

## Features

* Building and running on Linux and Windows.
* Basic support of GLTF models (No animations yet, some features missing, no draco encoding)
* HDR support and image based lighting
* Directional light with Cascaded Shadow Mapping
* Basic GUI with DearImGui with widgets
* Many physically based rendering features
* Deferred rendering with OpenGL backend
* Entity Component System
* Basic framework architecture
* Editor featuring .gltf and .hdr file loading and a simple camera controller
* Profiling with [Tracy](https://github.com/wolfpld/tracy)

## Requirements

* Python Version 3.x
* minimum CMake Version 3.10
* some c++11 compatible compiler
* optional: Doxygen and Graphviz for generating the documentation (Make sure everything is in the PATH and functional)

## Build

For an easy build you (hopefully) just need to run the ```create_solution.py``` script.
This will query all dependencies and populate the dependencies folder with the dependencies mentioned down below.
The script should also run ```cmake``` and the correct ```make```.

There are a few extra options in the cmake configuration:
* MANGO_BUILD_DOC (Default ON): This builds the documentation for mango.
* MANGO_BUILD_TESTS (Default OFF): This enables the "Testing Mode" in mango and builds the tests. This should ONLY be enabled, if you plan to run the tests. It enables and disables functionalities in mango.
* MANGO_ENABLE_HARD_WARNINGS (Default OFF): This enables some warning compiler flags. Attention: This could cause Mango to stop building.
* MANGO_PROFILE (Default OFF): This enables profiling mode that can be used to profile Mango with Tracy. You will need to install the Tracy-0.7.0 Visual Profiler from [here](https://github.com/wolfpld/tracy/releases/tag/v0.7).

If the dependency population fails, you might as well just try to populate the directory by yourself.
If ```cmake``` or ```make``` fails, you might as well just try to build it manually like that:

```console
mkdir build
cd build
cmake -G "Insert Generator Here" ..
```

After that it should be possible to build with some make derivate or under Windows with a Visual Studio solution.

If you still have a problem just reach out to me on GitHub and I'll try to fix it.

## Usage

Just head into ```\build``` and run ```make install``` to install mango.
There is a default install path ```install\mango\bin``` where the editor executable can be found. But you can also specify another one.
As an alternative you could just go to ```\build\debug\bin``` or ```\build\release\bin``` and run the editor executable, but keep in mind that you'll have to copy the ```\res``` folder there to get it to work properly. Also there seem to be some slowdowns when the file dialog is opened with attached debugger.

## Roadmap (unordered and incomplete)

* Implementing the compilation and saving of scenes (own format and gltf export)
* More lights
* Support for GLTF animations
* Support for translucent materials
* Rendering improvements (performance and appearance)
* Scene management improvements
* More performance measurement tools
* Some better examples for the use of Mango
* Many features like shadows, global light effects (GI, AO, reflections), live editing, hot swapable resources, and more

## Dependencies

* GLAD
* GLFW3
* glm
* spdlog
* tiny_gltf
* stb_image
* dear imgui
* tiny file dialogs
* optional

* tracy (Profiling)
* googletest (Testing)
* Doxygen (Documentation)

## Contribution
See the [guidelines](https://github.com/Paul-Hi/Mango/blob/master/CONTRIBUTING.md).

## Images
### Material Playground
![alt text](https://github.com/Paul-Hi/Mango/blob/master/show/material_playground.png "Material Playground")
### Sponza
![alt text](https://github.com/Paul-Hi/Mango/blob/master/show/sponza.png "Sponza")
### Boom Box
![alt text](https://github.com/Paul-Hi/Mango/blob/master/show/boom_box.png "Boom Box")
### Metal Rough Spheres
![alt text](https://github.com/Paul-Hi/Mango/blob/master/show/metal_rough_spheres.png "Metal Rough Spheres")
### Water Bottle
![alt text](https://github.com/Paul-Hi/Mango/blob/master/show/water_bottle.png "Water Bottle")
### Shaderball
![alt text](https://github.com/Paul-Hi/Mango/blob/master/show/shaderball.png "Shaderball")
