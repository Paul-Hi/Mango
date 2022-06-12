import os, subprocess

def runSubprocessVerbose(args):
    print(*args)
    process = subprocess.run(args, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, check=True)
    print(process.stdout.decode('utf8'))
    return process.returncode

def runGitSubmoduleCommands():
    result = runSubprocessVerbose(['git', 'submodule', 'init'])
    if result != 0:
        return False
    result = runSubprocessVerbose(['git', 'submodule', 'update'])
    if result != 0:
        return False
    return True

def createDependencyFiles():
    os.chdir('dependencies')

    # glad gen
    os.chdir('glad')

    gladGenCmd = ['python', '-m', 'glad', '--generator=c', '--spec=gl', '--profile=core', '--out-path=.', '--reproducible']
    result = runSubprocessVerbose(gladGenCmd)
    if result != 0:
        return False

    os.chdir('..')

    f = open('./glad/CMakeLists.txt','w+')
    f.write('project(glad)\r\n')
    f.write('add_library(glad src/glad.c include/glad/glad.h include/KHR/khrplatform.h)\r\n')
    f.write('target_include_directories(glad SYSTEM PUBLIC include)\r\n')
    f.close()

    #stb CMake

    f = open('./stb/CMakeLists.txt','w+')
    f.write('project(stb)\r\n')
    f.write('add_library(stb INTERFACE)\r\n')
    f.write('target_include_directories(stb SYSTEM INTERFACE .)\r\n')
    f.close()

    # dear imgui CMake
    f = open('./imgui/CMakeLists.txt','w+')
    f.write('project(imgui)\r\n')
    f.write('add_library(imgui imgui.cpp imgui_demo.cpp imgui_draw.cpp imgui_widgets.cpp imgui_tables.cpp imgui.h imconfig.h imgui_internal.h imstb_rectpack.h imstb_textedit.h imstb_truetype.h)\r\n')
    f.write('target_include_directories(imgui SYSTEM PUBLIC .)\r\n')
    f.close()

    # tiny file dialogs CMake
    f = open('./libtinyfiledialogs/CMakeLists.txt','w+')
    f.write('project(tinyfd)\r\n')
    f.write('add_library(tinyfd tinyfiledialogs.c tinyfiledialogs.h)\r\n')
    f.write('target_include_directories(tinyfd SYSTEM PUBLIC .)\r\n')
    f.close()

    # tracy CMake
    f = open('./tracy/CMakeLists.txt','w+')
    f.write('project(tracy)\r\n')
    f.write('add_library(tracy TracyClient.cpp TracyOpenGL.hpp)\r\n')
    f.write('target_link_libraries(tracy PUBLIC $<$<BOOL:${WIN32}>:wsock32> $<$<BOOL:${WIN32}>:ws2_32> $<$<BOOL:${WIN32}>:dbghelp>)\r\n')
    f.write('target_include_directories(tracy SYSTEM PUBLIC .)\r\n')
    f.write('target_compile_definitions(tracy PUBLIC $<$<CONFIG:Release>:$<$<BOOL:${MANGO_PROFILE}>:TRACY_ENABLE>> $<$<BOOL:${WIN32}>:WINVER=0x0601 _WIN32_WINNT=0x0601 _WINSOCKAPI_>)\r\n')
    f.close()

    os.chdir('..')
    return True

def main():
    if not runGitSubmoduleCommands():
        print('Failed initializing git submodules.')
        os._exit(1)
    if not createDependencyFiles():
        print('Failed creating generated files.')
        os._exit(1)

    os._exit(0)

if __name__== '__main__':
   main()
