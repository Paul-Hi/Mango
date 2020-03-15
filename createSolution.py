import os, subprocess, sys, getopt, errno

def getDependencies():
    if os.path.exists("dependencies"):
        return True
    os.mkdir("dependencies")
    os.chdir("dependencies")

    repository = "https://github.com/glfw/glfw.git"
    folder = "glfw"
    gitCmd = ['git', 'clone', repository, folder]
    result = subprocess.check_call(gitCmd, stderr=subprocess.STDOUT, shell=True)
    if result != 0:
        return False

    repository="https://github.com/Dav1dde/glad.git"
    folder="glad/gen"
    gitCmd = ['git', 'clone', repository, folder]
    result = subprocess.check_call(gitCmd, stderr=subprocess.STDOUT, shell=True)
    if result != 0:
        return False

    os.chdir("glad/gen")

    gladGenCmd = ['python', '-m', 'glad', '--generator=c', '--spec=gl', '--out-path=..', '--reproducible']
    result = subprocess.check_call(gladGenCmd, stderr=subprocess.STDOUT, shell=True)
    if result != 0:
        return False

    os.chdir("../..")

    f= open("./glad/CMakeLists.txt","w+")
    f.write("project(glad)\r\n")
    f.write("add_library(glad src/glad.c include/glad/glad.h include/KHR/khrplatform.h)\r\n")
    f.write("target_include_directories(glad PUBLIC include)\r\n")
    f.close()

    repository="https://github.com/google/googletest.git"
    folder="googletest"

    gitCmd = ['git', 'clone', repository, folder]
    result = subprocess.check_call(gitCmd, stderr=subprocess.STDOUT, shell=True)
    if result != 0:
        return False

    os.chdir("..")
    return True


def tryRunningMake():
    makes = ["make", "nmake", "mingw32-make.exe"]
    for make in makes:
        makeCmd = [make, '-j']
        status, _ = subprocess.getstatusoutput(makeCmd)
        if status != 0:
            continue
        else:
            break

    print ("Using " + makeCmd[0] + ".")
    result = subprocess.check_call(makeCmd, stderr=subprocess.STDOUT, shell=True)
    if result == 0:
        print ("Done building Mango.")
        return
    else:
        print ("Failed building Mango.")
        input("PRESS ENTER TO CONTINUE.")
        os._exit(1)


def main(argv):
    cmakeGen = ''
    try:
        opts, _ = getopt.getopt(argv, "hg:o", ["cmakeGenerator="])
    except getopt.GetoptError:
        print ("createSolution.py -g <cmakeGenerator>")
        os._exit(1)
    for opt, arg in opts:
        if opt == '-h':
            print ("createSolution.py -g <cmakeGenerator>")
            print ("Options are:")
            print ("    Visual Studio 16 2019            = Generates Visual Studio 2019 project files.")
            print ("    Visual Studio 15 2017            = Generates Visual Studio 2017 project files.")
            print ("    Visual Studio 14 2015            = Generates Visual Studio 2015 project files.")
            print ("    Visual Studio 12 2013            = Generates Visual Studio 2013 project files.")
            print ("    Visual Studio 11 2012            = Generates Visual Studio 2012 project files.")
            print ("    Visual Studio 10 2010            = Generates Visual Studio 2010 project files.")
            print ("    Visual Studio 9 2008             = Generates Visual Studio 2008 project files.")
            print ("    Borland Makefiles                = Generates Borland makefiles.")
            print ("    NMake Makefiles                  = Generates NMake makefiles.")
            print ("    NMake Makefiles JOM              = Generates JOM makefiles.")
            print ("    MSYS Makefiles                   = Generates MSYS makefiles.")
            print ("    MinGW Makefiles                  = Generates a make file for use with mingw32-make.")
            print ("    Unix Makefiles                   = Generates standard UNIX makefiles.")
            print ("    Green Hills MULTI                = Generates Green Hills MULTI files.")
            print ("    Ninja                            = Generates build.ninja files.")
            print ("    Watcom WMake                     = Generates Watcom WMake makefiles.")
            print ("    CodeBlocks - MinGW Makefiles     = Generates CodeBlocks project files.")
            print ("    CodeBlocks - NMake Makefiles     = Generates CodeBlocks project files.")
            print ("    CodeBlocks - NMake Makefiles JOM = Generates CodeBlocks project files.")
            print ("    CodeBlocks - Ninja               = Generates CodeBlocks project files.")
            print ("    CodeBlocks - Unix Makefiles      = Generates CodeBlocks project files.")
            print ("    CodeLite - MinGW Makefiles       = Generates CodeLite project files.")
            print ("    CodeLite - NMake Makefiles       = Generates CodeLite project files.")
            print ("    CodeLite - Ninja                 = Generates CodeLite project files.")
            print ("    CodeLite - Unix Makefiles        = Generates CodeLite project files.")
            print ("    Sublime Text 2 - MinGW Makefiles = Generates Sublime Text 2 project files.")
            print ("    Sublime Text 2 - NMake Makefiles = Generates Sublime Text 2 project files.")
            print ("    Sublime Text 2 - Ninja           = Generates Sublime Text 2 project files.")
            print ("    Sublime Text 2 - Unix Makefiles  = Generates Sublime Text 2 project files.")
            print ("    Kate - MinGW Makefiles           = Generates Kate project files.")
            print ("    Kate - NMake Makefiles           = Generates Kate project files.")
            print ("    Kate - Ninja                     = Generates Kate project files.")
            print ("    Kate - Unix Makefiles            = Generates Kate project files.")
            print ("    Eclipse CDT4 - NMake Makefiles   = Generates Eclipse CDT 4.0 project files.")
            print ("    Eclipse CDT4 - MinGW Makefiles   = Generates Eclipse CDT 4.0 project files.")
            print ("    Eclipse CDT4 - Ninja             = Generates Eclipse CDT 4.0 project files.")
            print ("    Eclipse CDT4 - Unix Makefiles    = Generates Eclipse CDT 4.0 project files.")
            os._exit(0)
        elif opt in ("-g", "--cmakeGenerator"):
            cmakeGen = arg
    print ("The generator for cmake is " + cmakeGen)



    print ("Loading dependencies.")
    if getDependencies():
        print("Done loading dependencies.")
    else:
        print("Failed loading dependencies.")
        input("PRESS ENTER TO CONTINUE.")
        os._exit(1)

    print ("Running CMake.")
    if not os.path.exists("build"):
        os.mkdir("build")
    os.chdir("build")

    cmakeCmd = ['cmake', '-G', cmakeGen, '..']
    result = subprocess.check_call(cmakeCmd, stderr=subprocess.STDOUT, shell=True)
    if result == 0:
        print ("Done running CMake.")
    else:
        print ("Failed running CMake.")
        input("PRESS ENTER TO CONTINUE.")
        os._exit(1)


    print ("Building Mango.")
    tryRunningMake()

    os.chdir("..")
    input("PRESS ENTER TO CONTINUE.")


if __name__== "__main__":
   main(sys.argv[1:])
