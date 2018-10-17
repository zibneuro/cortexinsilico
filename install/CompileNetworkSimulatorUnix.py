#! /usr/bin/python3

from subprocess import check_call
from subprocess import Popen, PIPE
import os
import sys


def compileCppAndRunUnitTests(src_dir, build_dir):
    print("[*] Compiling c++ programs")

    networkSimulator_pro_file = os.path.join(src_dir, "networkSimulator/networkSimulator.pro")
    networkSimulator_build_dir = os.path.join(build_dir, "networkSimulator")
    os.makedirs(networkSimulator_build_dir, exist_ok=True)
    check_call(["qmake", networkSimulator_pro_file], cwd=networkSimulator_build_dir)
    check_call(["make", "all"], cwd=networkSimulator_build_dir)

    print("\n")


def installAptPackage(package):
    try:
        ps = Popen(('dpkg', '-s', package), stdout=PIPE)
        check_call(('grep', 'Status'), stdin=ps.stdout)
        ps.wait()
        print("[*] {0} already available".format(package))
    except:
        print("[*] Installing {0}".format(package))
        check_call(["sudo", "apt-get", "install", "--yes", package])


def printUsageAndExit():
    print("Usage:")
    print("cd cortexinsilico-buildingbrains")
    print("python3 install/CompileAndRunTests.py")
    sys.exit(1)


repo_root = os.getcwd()
src_dir = os.path.join(repo_root, 'src')
build_dir = os.path.join(repo_root, 'build')

if not os.path.isdir(src_dir):
    print("Cannot find directory 'src'. Are you in the correct directory?")
    printUsageAndExit()

os.makedirs(build_dir, exist_ok=True)

installAptPackage('gcc')
installAptPackage('qt5-default')
installAptPackage('qt5-qmake')

compileCppAndRunUnitTests(src_dir, build_dir)

print("Done.")
