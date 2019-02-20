import sys
import subprocess
import os

EPS = "0.00001"

def printUsageAndExit():
    print("Usage:")
    print("python3 parseSpatialFolder.py <dir> <outfile>")
    print()
    print("<dir> = The directory with the downloaded spatial innervation distribution.")    
    sys.exit(1)

if (len(sys.argv) != 3):
    printUsageAndExit()

refDir = sys.argv[1]
outfile = sys.argv[2]

neuronToNeuronInnervation = []

resultFile = open(outfile,"w+")

for root, dirs, files in os.walk(refDir):
    for file in files:
        if "preNeuronID" in file:
            filepath = os.path.join(root,file)
            preID = int(file.split("_")[1])
            with open(filepath) as f:
                lines = f.readlines()
                innervation = 0
                postID = -1                
                for line in lines:
                    first = line.split(" ")[0]
                    second = line.split(" ")[1]
                    if "postNeuronID" in first: 
                        if postID != -1:                            
                            resultFile.write(str(preID))
                            resultFile.write(" ")
                            resultFile.write(str(postID))
                            resultFile.write(" ")
                            resultFile.write(str(innervation))
                            resultFile.write("\n")
                        innervation = 0
                        postID = int(second)
                    else:
                        innervation += float(second)

resultFile.close()

