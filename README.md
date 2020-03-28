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

* The build runs :D
* Some functioning logging and assertion capabilities.
* Window creation with glfw is possible.

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

* Base Structure
* Deferred RenderPipeline with OpenGL Backend
* Physically Based Rendering
* Scene Description
* Entity Component System and/or some graph structure
* GUI integrating DearImGui
* Editor and ExampleApplications

## Dependencies

* GLAD
* GLFW3
* spdlog
* stb_image

* googletest (Testing)
* Doxygen (Documentation)
