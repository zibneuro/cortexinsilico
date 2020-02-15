#! /usr/bin/python3

from subprocess import check_call
from subprocess import Popen, PIPE
import os
import sys


def compileCppAndRunUnitTests(src_dir, build_dir):
    print("[*] Compiling c++ programs")

    process3DQuery_pro_file = os.path.join(src_dir, "processCIS3DQuery/processCIS3DQuery.pro")
    process3DQuery_build_dir = os.path.join(build_dir, "processCIS3DQuery")
    os.makedirs(process3DQuery_build_dir, exist_ok=True)
    check_call(["qmake", process3DQuery_pro_file], cwd=process3DQuery_build_dir)
    check_call(["make", "all", "-j", "4"], cwd=process3DQuery_build_dir)

    inputmapper_pro_file = os.path.join(src_dir, "inputmapper/inputmapper.pro")
    inputmapper_build_dir = os.path.join(build_dir, "inputmapper")
    os.makedirs(inputmapper_build_dir, exist_ok=True)
    check_call(["qmake", inputmapper_pro_file], cwd=inputmapper_build_dir)
    check_call(["make", "all", "-j", "4"], cwd=inputmapper_build_dir)

    test_pro_file = os.path.join(src_dir, "test/runTests.pro")
    test_build_dir = os.path.join(build_dir, "test")
    os.makedirs(test_build_dir, exist_ok=True)
    check_call(["qmake", test_pro_file], cwd=test_build_dir)
    check_call(["make", "all", "-j", "4"], cwd=test_build_dir)

    print("[*] Running unit tests")
    check_call(["./release/runTests"], cwd=test_build_dir)
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
