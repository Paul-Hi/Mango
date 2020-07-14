import os, subprocess, sys, getopt, errno

def getDependencies():
    if os.path.exists('dependencies'):
        return True
    os.mkdir('dependencies')
    os.chdir('dependencies')

    # glfw
    repository = 'https://github.com/glfw/glfw.git'
    folder = 'glfw'
    gitCmd = ['git', 'clone', repository, folder]
    result = subprocess.check_call(gitCmd, stderr=subprocess.STDOUT, shell=False)
    if result != 0:
        return False

    # glad
    repository ='https://github.com/Dav1dde/glad.git'
    folder ='glad/gen'
    gitCmd = ['git', 'clone', repository, folder]
    result = subprocess.check_call(gitCmd, stderr=subprocess.STDOUT, shell=False)
    if result != 0:
        return False

    os.chdir('glad/gen')

    gladGenCmd = ['python', '-m', 'glad', '--generator=c', '--spec=gl', '--out-path=..', '--reproducible']
    result = subprocess.check_call(gladGenCmd, stderr=subprocess.STDOUT, shell=False)
    if result != 0:
        return False

    os.chdir('../..')

    f = open('./glad/CMakeLists.txt','w+')
    f.write('project(glad)\r\n')
    f.write('add_library(glad src/glad.c include/glad/glad.h include/KHR/khrplatform.h)\r\n')
    f.write('target_include_directories(glad PUBLIC include)\r\n')
    f.close()

    # googletest
    repository ='https://github.com/google/googletest.git'
    folder ='googletest'

    gitCmd = ['git', 'clone', repository, folder]
    result = subprocess.check_call(gitCmd, stderr=subprocess.STDOUT, shell=False)
    if result != 0:
        return False

    # spdlog
    repository ='https://github.com/gabime/spdlog.git'
    folder ='spdlog'

    gitCmd = ['git', 'clone', repository, folder]
    result = subprocess.check_call(gitCmd, stderr=subprocess.STDOUT, shell=False)
    if result != 0:
        return False

    # stb_image
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
    f.write('target_include_directories(stb_image INTERFACE .)\r\n')
    f.close()

    # glm
    repository ='https://github.com/g-truc/glm.git'
    folder ='glm'

    gitCmd = ['git', 'clone', repository, folder]
    result = subprocess.check_call(gitCmd, stderr=subprocess.STDOUT, shell=False)
    if result != 0:
        return False

    # tiny_gltf
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
    f.write('target_include_directories(tiny_gltf INTERFACE .)\r\n')
    f.close()


    os.chdir('..')
    return True


def tryRunningMake():
    makes = ['make', 'nmake', 'mingw32-make', 'ninja']
    for make in makes:
        makeCmd = [make]
        print ('Trying ' + makeCmd[0] + '.')
        try:
            result = subprocess.check_call(makeCmd, stderr=subprocess.STDOUT, shell=False)
        except:
            continue

        if result == 0:
            print ('Done building Mango.')
            return
        else:
            continue

    if result != 0:
        print ('Can not get the available make derivate or failed building Mango.')
        print ('Please run your \'make\' in the build directory!')
        input('PRESS ENTER TO CONTINUE.')
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
        input('PRESS ENTER TO CONTINUE.')
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
        input('PRESS ENTER TO CONTINUE.')
        os._exit(1)


    print ('Building Mango.')
    tryRunningMake()

    os.chdir('..')
    input('PRESS ENTER TO CONTINUE.')


if __name__== '__main__':
   main(sys.argv[1:])
