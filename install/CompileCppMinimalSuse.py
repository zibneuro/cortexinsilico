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

    convertAxon_pro_file = os.path.join(src_dir, "convertAxonRedundancyMap/convertAxonRedundancyMap.pro")
    convertAxon_build_dir = os.path.join(build_dir, "convertAxonRedundancyMap")
    os.makedirs(convertAxon_build_dir, exist_ok=True)
    check_call(["qmake", convertAxon_pro_file], cwd=convertAxon_build_dir)
    check_call(["make", "all"], cwd=convertAxon_build_dir)

    convertSparse_pro_file = os.path.join(src_dir, "convertSparseField/convertSparseField.pro")
    convertSparse_build_dir = os.path.join(build_dir, "convertSparseField")
    os.makedirs(convertSparse_build_dir, exist_ok=True)
    check_call(["qmake", convertSparse_pro_file], cwd=convertSparse_build_dir)
    check_call(["make", "all"], cwd=convertSparse_build_dir)

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

compileCppAndRunUnitTests(src_dir, build_dir)

print("Done.")
