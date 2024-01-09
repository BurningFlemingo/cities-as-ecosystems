import os
import subprocess
import sys


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
    preset: str = sys.argv[1]
    buildDir = buildPath(rootDir, "build", preset)

    os.makedirs(buildPath(buildDir, "shaders"), exist_ok=True)

    cmakeCommand = f"cmake --preset {preset} -S {rootDir}"
    ninjaCommand = f"pushd {buildDir} && ninja && popd"

    commands = [
        cmakeCommand,
        ninjaCommand
    ]

    for command in commands:
        runCommand(command)
