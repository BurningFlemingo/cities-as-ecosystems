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

    os.makedirs(buildDir, exist_ok=True)

    platform = os.name  # 'posix' for Linux and Mac, 'nt' for Windows
    if platform == "posix":
        vcpkgBootstrap = f"{buildPath(vcpkgDir, 'bootstrap-vcpkg.sh')}"
        vcpkgInstall = f"{buildPath(vcpkgDir, 'vcpkg')} install sdl2[core,vulkan] glm vulkan --recurse"
    elif platform == "nt":
        vcpkgBootstrap = f"{buildPath(vcpkgDir, 'bootstrap-vcpkg.bat')}"
        vcpkgInstall = f"{buildPath(vcpkgDir, 'vcpkg.exe')} install sdl2[core,vulkan] glm vulkan --recurse"
    else:
        print(f"Unsupported platform: {platform}")
        exit()

    runCommand(vcpkgBootstrap)
    runCommand(vcpkgInstall)
