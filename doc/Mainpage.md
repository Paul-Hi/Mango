<img src="https://raw.githubusercontent.com/Paul-Hi/Mango/master/res/textures/logo.png" alt="alt text" width="5%" style="float:right; margin:1%"/>
# Mango

Mango is an Open Source Framework. Or at least it should be in the future.
A playground for people like me.

Just to be clear: This is not the first attempt!
I plan to implement various features, with main focus on computer graphics.
A few of them are described below, but there will be added a lot more in the future.
There should also always be a documentation and tests to ensure cleaner and more functional code.

If you have suggestions for improvement or find a bug just let me know.
Keep in mind that I do this in my spare time, besides studying and working.

## Features

* Building and running on Linux and Windows.
* Basic support of GLTF models (No animations yet, some features missing, no draco encoding)
* HDR support and image based lighting
* Many physically based rendering features
* Deferred rendering with OpenGL backend
* Entity Component System
* Basic framework architecture
* Editor featuring .gltf and .hdr file loading via drag'n'drop and a simple camera controller

## Requirements

* Python Version 3.x
* minimum CMake Version 3.10
* some c++11 compatible compiler
* optional: Doxygen and Graphviz for generating the documentation (Make sure everything is in the PATH and functional)

## Build

For an easy build you (hopefully) just need to run the ```create_solution.py``` script.
This will query all dependencies and populate the dependencies folder with the dependencies mentioned down below.
The script should also run ```cmake``` and the correct ```make```.

There are two extra options in the cmake configuration:
* MANGO_BUILD_DOC (Default ON): This builds the documentation for mango.
* MANGO_BUILD_TESTS (Default OFF): This enables the "Testing Mode" in mango and builds the tests. This should ONLY be enabled, if you plan to run the tests. It enables and disables functionalities in mango.
* MANGO_ENABLE_HARD_WARNINGS (Default OFF): This enables some warning compiler flags. Attention: This will cause Mango to stop building, as even some libraries do not build without errors.

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
As an alternative you could just go to ```\build\debug\bin``` or ```\build\release\bin``` and run the editor executable, but keep in mind that you'll have to copy the ```\res``` folder there to get it to work properly.

## Roadmap (unordered and incomplete)

* Implementing the compilation and saving of scenes (own format and gltf export)
* Support for GLTF animations
* Support for translucent materials
* Rendering improvements (performance and appearance)
* Scene management improvements
* GUI with DearImGui
* Performance measurement tools
* Editor improvements and some better examples for the use of Mango
* Many features like shadows, global light effects (GI, AO, reflections), live editing, hot swapable resources, and more

## Dependencies

* GLAD
* GLFW3
* glm
* spdlog
* tiny_gltf
* stb_image

* googletest (Testing)
* Doxygen (Documentation)

## Images
### Water Bottle
<img src="https://raw.githubusercontent.com/Paul-Hi/Mango/master/show/water_bottle.png" alt="alt text" width="75%"/>
### Boom Box
<img src="https://raw.githubusercontent.com/Paul-Hi/Mango/master/show/boom_box.png" alt="alt text" width="75%"/>
### Metal Rough Spheres
<img src="https://raw.githubusercontent.com/Paul-Hi/Mango/master/show/metal_rough_spheres.png" alt="alt text" width="75%"/>