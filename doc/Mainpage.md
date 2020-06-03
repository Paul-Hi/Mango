# Mango - Graphics Engine

Mango is an Open Source Graphics Engine. Or at least it should be in the future.
A kind of playground for people like me.

Just to be clear: This is not the first try!
I plan to implement various features, with main focus on computer graphics.
A few of them are described below, but there will be added a lot more in the future.
There should also always be a documentation and tests to ensure cleaner and more functional code.

If you have suggestions for improvement or find a bug just let me know.
Keep in mind that I do this in my spare time, besides studying and working.

## Features

* Basic GLTF loading and rendering (No animations yet, some features missing, no draco encoding).
* HDR loading and rendering including image based lighting.
* Many physically based rendering features.
* Deferred rendering with command buffer and OpenGL backend.
* Entity Component System.
* Basic framework architecture.
* Editor supporting gltf and hdr loading via drag'n'drop and a simple camera controller.

## Requirements

* Python Version 3.x
* minimum CMake Version 3.10
* some c++11 compatible compiler using the posic thread model.
* optional: Doxygen for generating the documentation

## Build

For an easy build you (hopefully) just need to run the ```create_solution.py``` script.
This will query all dependencies and populate the dependencies folder.
The script should also run ```cmake``` and the correct ```make```.

There are two extra options in the cmake configuration:
* MANGO_BUILD_TESTS (Default OFF): This enables the "Testing Mode" in mango and builds the tests. This should ONLY be enabled, if you plan to run the tests. It enables and disables functionalities in mango.
* MANGO_BUILD_DOC (Default ON): This builds the documentation for mango.

If the dependency population fails you could try to populate the directory by yourself.
If ```cmake``` or ```make``` fails you could also try to build it manually like that:

```console
mkdir build
cd build
cmake -G "Insert Generator Here" ..
make
```

Make sure that you use the right generator for cmake and an installed make.
If you still have a problem just reach out to me on GitHub and I'll try to fix it.

## Usage

Just head into ```\build\debug\bin``` or ```\build\release\bin``` and run the editor executable.

## Roadmap (unordered and incomplete)

* Scene composing and store.
* GLTF animations.
* Transparency.
* Improved rendering.
* Improved scene management.
* GUI integrating DearImGui.
* Performance tools.
* Better editor and some example applications.
* Many features like shadows, global light effects (GI, AO, reflections), live editing, hot swapable resources, and more.

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
### Damaged Helmet
![alt text](https://github.com/Paul-Hi/Mango/blob/master/show/damaged_helmet.png "Damaged Helmet")
### Boom Box
![alt text](https://github.com/Paul-Hi/Mango/blob/master/show/boom_box.png "Boom Box")
### Water Bottle
![alt text](https://github.com/Paul-Hi/Mango/blob/master/show/water_bottle.png "Water Bottle")
### Metal Rough Spheres
![alt text](https://github.com/Paul-Hi/Mango/blob/master/show/metal_rough_spheres.png "Metal Rough Spheres")