import os
import subprocess


def runCommand(command):
    print(f"\tExecuting command: {command}")
    process = subprocess.Popen(command, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)

    returnCode = process.returncode
    out, err = process.communicate()

    out = out.decode("utf-8")
    err = err.decode("utf-8")

    print(f"\tCommand execution completed with return code: {returnCode}\n")
    if out:
        print(f"Output:\n{out}")
    if err:
        print(f"Error:\n{err}")


def buildPath(rootDir, *subdirs):
    return os.path.join(rootDir, *subdirs)


if __name__ == "__main__":
    rootDir = os.path.join(os.path.dirname(os.path.realpath(__file__)), ".." + os.sep)
    buildDir = buildPath(rootDir, "build")
    vcpkgDir = buildPath(rootDir, "vcpkg")
    srcDir = buildPath(rootDir, "src")

    os.makedirs(buildPath(buildDir, "shaders"), exist_ok=True)

    glslcCommandVert = f"glslc {buildPath(srcDir, 'shaders', 'first.vert')} -o {buildPath(buildDir, 'shaders', 'first_vert.spv')}"
    glslcCommandFrag = f"glslc {buildPath(srcDir, 'shaders', 'first.frag')} -o {buildPath(buildDir, 'shaders', 'first_frag.spv')}"

    if os.path.exists(vcpkgDir):
        cmakeVcpkgToolchainArg = f"-DCMAKE_TOOLCHAIN_FILE={buildPath(vcpkgDir, 'scripts', 'buildsystems', 'vcpkg.cmake')}"
    else:
        cmakeVcpkgToolchainArg = ""

    cmakeCommand = f"cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=1 -B {buildDir} -S {rootDir} {cmakeVcpkgToolchainArg}  -G Ninja"
    ninjaCommand = f"pushd {buildDir} && ninja && popd"

    commands = [
        glslcCommandVert,
        glslcCommandFrag,
        cmakeCommand,
        ninjaCommand
    ]

    for command in commands:
        runCommand(command)
