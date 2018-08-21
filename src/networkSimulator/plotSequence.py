import os
import subprocess

index = 1
maxRange = "10"
for filename in os.listdir(os.getcwd()):
    imageName = "{0:0>5}".format(index)
    print(imageName)
    subprocess.run(["p3", "plot.py"], cwd=os.getcwd)
    index += 1
    