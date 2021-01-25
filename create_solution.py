import os, subprocess, sys, getopt, errno

def getDependencies():
    if not os.path.exists('dependencies'):
        os.mkdir('dependencies')
    os.chdir('dependencies')

    # glfw
    if not os.path.exists('glfw'):
        repository = 'https://github.com/glfw/glfw.git'
        folder = 'glfw'
        gitCmd = ['git', 'clone', repository, folder]
        result = subprocess.check_call(gitCmd, stderr=subprocess.STDOUT, shell=False)
        if result != 0:
            return False

    # glad
    if not os.path.exists('glad'):
        repository ='https://github.com/Dav1dde/glad.git'
        folder ='glad/gen'
        gitCmd = ['git', 'clone', repository, folder]
        result = subprocess.check_call(gitCmd, stderr=subprocess.STDOUT, shell=False)
        if result != 0:
            return False

        os.chdir('glad/gen')

        gladGenCmd = ['python', '-m', 'glad', '--generator=c', '--spec=gl', '--profile=core', '--out-path=..', '--reproducible']
        result = subprocess.check_call(gladGenCmd, stderr=subprocess.STDOUT, shell=False)
        if result != 0:
            return False

        os.chdir('../..')

        f = open('./glad/CMakeLists.txt','w+')
        f.write('project(glad)\r\n')
        f.write('add_library(glad src/glad.c include/glad/glad.h include/KHR/khrplatform.h)\r\n')
        f.write('target_include_directories(glad SYSTEM PUBLIC include)\r\n')
        f.close()

    # googletest
    if not os.path.exists('googletest'):
        repository ='https://github.com/google/googletest.git'
        folder ='googletest'

        gitCmd = ['git', 'clone', repository, folder]
        result = subprocess.check_call(gitCmd, stderr=subprocess.STDOUT, shell=False)
        if result != 0:
            return False

    # spdlog
    if not os.path.exists('spdlog'):
        repository ='https://github.com/gabime/spdlog.git'
        folder ='spdlog'

        gitCmd = ['git', 'clone', repository, folder]
        result = subprocess.check_call(gitCmd, stderr=subprocess.STDOUT, shell=False)
        if result != 0:
            return False

    # stb_image
    if not os.path.exists('stb_image'):
        repository ='https://github.com/nothings/stb.git'
        folder ='stb_image'

        gitCmd = ['git', 'clone', '-n', repository, folder]
        result = subprocess.check_call(gitCmd, stderr=subprocess.STDOUT, shell=False)
        if result != 0:
            return False

        os.chdir('stb_image')

        gitCmd = ['git', 'checkout', 'HEAD', 'stb_image.h', 'stb_image_write.h']
        result = subprocess.check_call(gitCmd, stderr=subprocess.STDOUT, shell=False)
        if result != 0:
            return False

        os.chdir('..')

        f = open('./stb_image/CMakeLists.txt','w+')
        f.write('project(stb_image)\r\n')
        f.write('add_library(stb_image INTERFACE)\r\n')
        f.write('target_include_directories(stb_image SYSTEM INTERFACE .)\r\n')
        f.close()

    # glm
    if not os.path.exists('glm'):
        repository ='https://github.com/g-truc/glm.git'
        folder ='glm'

        gitCmd = ['git', 'clone', repository, folder]
        result = subprocess.check_call(gitCmd, stderr=subprocess.STDOUT, shell=False)
        if result != 0:
            return False

    # tiny_gltf
    if not os.path.exists('tiny_gltf'):
        repository ='https://github.com/syoyo/tinygltf.git'
        folder ='tiny_gltf'

        gitCmd = ['git', 'clone', '-n', repository, folder]
        result = subprocess.check_call(gitCmd, stderr=subprocess.STDOUT, shell=False)
        if result != 0:
            return False

        os.chdir('tiny_gltf')

        gitCmd = ['git', 'checkout', 'HEAD', 'json.hpp', 'tiny_gltf.h']
        result = subprocess.check_call(gitCmd, stderr=subprocess.STDOUT, shell=False)
        if result != 0:
            return False

        os.chdir('..')

        f = open('./tiny_gltf/CMakeLists.txt','w+')
        f.write('project(tiny_gltf)\r\n')
        f.write('add_library(tiny_gltf INTERFACE)\r\n')
        f.write('target_include_directories(tiny_gltf SYSTEM INTERFACE .)\r\n')
        f.close()

    # dear imgui
    if not os.path.exists('imgui'):
        repository ='https://github.com/ocornut/imgui.git'
        folder ='imgui'

        gitCmd = ['git', 'clone', repository, folder, '--branch', 'docking']
        result = subprocess.check_call(gitCmd, stderr=subprocess.STDOUT, shell=False)
        if result != 0:
            return False

        f = open('./imgui/CMakeLists.txt','w+')
        f.write('project(imgui)\r\n')
        f.write('add_library(imgui imgui.cpp imgui_demo.cpp imgui_draw.cpp imgui_widgets.cpp imgui_tables.cpp imgui.h imconfig.h imgui_internal.h imstb_rectpack.h imstb_textedit.h imstb_truetype.h)\r\n')
        f.write('target_include_directories(imgui SYSTEM PUBLIC .)\r\n')
        f.close()

    # tiny file dialogs
    if not os.path.exists('tinyfd'):
        repository ='http://git.code.sf.net/p/tinyfiledialogs/code'
        folder ='tinyfd'

        gitCmd = ['git', 'clone', repository, folder]
        result = subprocess.check_call(gitCmd, stderr=subprocess.STDOUT, shell=False)
        if result != 0:
            return False

        f = open('./tinyfd/CMakeLists.txt','w+')
        f.write('project(tinyfd)\r\n')
        f.write('add_library(tinyfd tinyfiledialogs.c tinyfiledialogs.h)\r\n')
        f.write('target_include_directories(tinyfd SYSTEM PUBLIC .)\r\n')
        f.close()

    # tracy
    if not os.path.exists('tracy'):
        repository ='https://github.com/wolfpld/tracy.git'
        folder ='tracy'

        gitCmd = ['git', 'clone', '--depth', '1', '--branch', 'v0.7', repository, folder]
        result = subprocess.check_call(gitCmd, stderr=subprocess.STDOUT, shell=False)
        if result != 0:
            return False

        f = open('./tracy/CMakeLists.txt','w+')
        f.write('project(tracy)\r\n')
        f.write('add_library(tracy TracyClient.cpp TracyOpenGL.hpp)\r\n')
        f.write('target_link_libraries(tracy PUBLIC $<$<BOOL:${WIN32}>:wsock32> $<$<BOOL:${WIN32}>:ws2_32> $<$<BOOL:${WIN32}>:dbghelp>)\r\n')
        f.write('target_include_directories(tracy SYSTEM PUBLIC .)\r\n')
        f.write('target_compile_definitions(tracy PUBLIC $<$<CONFIG:Release>:$<$<BOOL:${MANGO_PROFILE}>:TRACY_ENABLE>> $<$<BOOL:${WIN32}>:WINVER=0x0601 _WIN32_WINNT=0x0601 _WINSOCKAPI_>)\r\n')
        f.close()

    os.chdir('..')
    return True


def tryRunningMake():
    makes = ['make', 'nmake', 'mingw32-make', 'ninja']
    result = 1
    for make in makes:
        makeCmd = [make]
        print ('Trying ' + makeCmd[0] + '.')
        try:
            result = subprocess.check_call(makeCmd, stderr=subprocess.STDOUT, shell=False)
        except:
            continue

        if result == 0:
            return True
        else:
            return False

    if result != 0:
        print ('Can not get the available \'make\' derivate.')
        print ('Please run your \'make\' in the build directory!')
        os._exit(1)


def main(argv):
    cmakeGen = ''
    try:
        opts, _ = getopt.getopt(argv, 'hG:', ['help', 'cmake_generator='])
    except getopt.GetoptError:
        print ('get help with: create_solution.py -h')
        print ('usage: create_solution.py -G <cmake_generator>')
        os._exit(1)
    for opt, arg in opts:
        if opt in ('-h', '--help'):
            print ('usage: create_solution.py -G <cmake_generator>')
            cmakeHelp = ['cmake', '-h']
            output = subprocess.check_output(cmakeHelp, universal_newlines=True, shell=False)
            generators = output.split('Generators', 1)[1]
            for row in generators.split('\n'):
                row = row.replace('[arch]', '      ')
                if not 'option' in row.lower():
                    print(row)
            os._exit(0)
        elif opt in ('-G', '--cmake_generator'):
            cmakeGen = arg

    if cmakeGen == '':
        cmakeCmd = ['cmake', '..']
        print ('The platform default cmake generator is used. This may fail.')
    else:
        cmakeCmd = ['cmake', '-G', cmakeGen, '..']
        print ('The generator for cmake is ' + cmakeGen)



    print ('Loading dependencies.')
    if getDependencies():
        print('Done loading dependencies.')
    else:
        print('Failed loading dependencies.')
        os._exit(1)

    print ('Running CMake.')
    if not os.path.exists('build'):
        os.mkdir('build')
    os.chdir('build')

    result = subprocess.check_call(cmakeCmd, stderr=subprocess.STDOUT, shell=False)
    if result == 0:
        print ('Done running CMake.')
    else:
        print ('Failed running CMake.')
        os._exit(1)


    print ('Building Mango.')
    result = tryRunningMake()
    if result:
        print ('Build successful!')
        os._exit(0)
    else:
        print ('Failed building Mango!')
        os._exit(1)

if __name__== '__main__':
   main(sys.argv[1:])
