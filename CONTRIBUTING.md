# Contributing to Mango

Input is always appreciated!
There are many things you can do, such as report bugs, talk about things you like or dislike, submit fixes or propose new features you would like to see.
This can all be done through Github!

## Checklist for a [pull request](https://github.com/Paul-Hi/Mango/pulls)

Always use pull requests to propose changes to the codebase. At the moment the desired procedure is as follows ([Github Flow](https://guides.github.com/introduction/flow/index.html)):

1. Fork the repository and create a branch from 'master'. Please choose a branch name that roughly describes topic you are working on.
2. If you add functionality that can be documented, please do so. If in doubt, it is better to document too much than too little.
3. Testing your code is always a good style.
4. Ensure that the coding style fits into the rest of Mango. See the guidelines below for more information.
5. Make sure that your build runs without errors and as few warnings as possible. It could be a good idea to run it with the CMake Option *MANGO_ENABLE_HARD_WARNINGS* enabled.
6. Describe everything you did in the pull request template. If you fixed a specific issue, please mention it!
7. Issue the [pull request](https://github.com/Paul-Hi/Mango/pulls)!

## Any contributions you make will be under the Apache License Version 2.0

The project is covered under the [Apache License 2.0](http://www.apache.org/licenses/LICENSE-2.0). All your submissions to the codebase are understood to be under the same license. Please contact me if you have any open questions in this regard.

## Bug reports and feature requests with Github [issues](https://github.com/Paul-Hi/Mango/issues)

Please open an [issue](https://github.com/Paul-Hi/Mango/issues) when you encounter any bugs or similar. What is unknown can not be fixed. Please use the corresponding template. Also open one when you would like to request a feature.
When reporting bugs, ensure that the description is clear and easy to understand. Include a quick summary, steps to reproduce as well as expected and actual behaviour. Also include sample code and screenshots if necessary.
Feature requests should include a detailed description and ideally a suggested solution and alternatives.

## Coding style guidelines

In general, the specifications in the [.clang-format](https://github.com/Paul-Hi/Mango/blob/master/.clang-format) file are the way to go. It is a lot easier to maintain the codebase, when everyone tries to follow the guidelines.

A few thing to mention here:

* Use clang format with the configuration file.
* Use snake_case in general. Only macros are upper-case.
* Use .hpp file endings for c++ headers and .cpp for the implementation.
* No pragmas. Do not forget to add include guards.
* Only add new namespaces if there really is no other way (There is one). Everything else should be in `namespace mango` . `using namespace mango` is fine, everything else is not.
* Files should include some documentation header. If it is missing it will be added later.
* 4 spaces for indentation and NO tabs.
* For reference how to handle .hpp files see [application.hpp](https://github.com/Paul-Hi/Mango/blob/master/mango/include/mango/application.hpp), for .cpp see [application.cpp](https://github.com/Paul-Hi/Mango/blob/master/mango/src/core/application.cpp).
* Just ask if something is unclear. This saves time!

## What else?

If you are nice to others, others will be nice to you.
Bugs don't just disappear. Be patient.
For more information regarding that topic I would like to refer to the [Code of Conduct](https://github.com/Paul-Hi/Mango/blob/master/CODE_OF_CONDUCT.md).

If you have any further questions, feel free to ask them.
