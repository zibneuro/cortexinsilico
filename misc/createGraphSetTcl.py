import os
import sys

def printUsageAndExit():
    print("Usage:")
    print("python createGraphSetTcl.py <networkSpecificationDir> <cellType> <colorR> <colorG> <colorB> <column> [<column> ...]")    
    print("")
    print("Example: python createGraphSetTcl.py C:\\Users\\ph\\networkSpecficationData 1 0 0 L4py C2 B2")
    print("")
    print("In Amira type: source C:/path/to/script.tcl")
    sys.exit(1)

if (len(sys.argv) < 7):
    printUsageAndExit()

networkDir = sys.argv[1]
cellType = sys.argv[2]
colorR = sys.argv[3]
colorG = sys.argv[4]
colorB = sys.argv[5]

columns = []
for i in range(6,len(sys.argv)):
    columns.append(sys.argv[i])

name = cellType
for column in columns:
    name = name + "_" + column
resultFileName = "loadGraphs_" + name + ".tcl"

with open(resultFileName, "w+") as f:         
    for column in columns:
        currentDir = os.path.join(networkDir, 'dendrites_ex', column, cellType)
        for (dirpath, dirnames, filenames) in os.walk(currentDir):
            index = 1
            for filename in filenames:
                if(".am" in filename):
                    fullFilename = os.path.join(currentDir, filename)        
                    fullFilename = fullFilename.replace("\\","/") 
                    f.write("remove " + filename + " \n")
                    f.write("load " + fullFilename + " \n")
                    graphName = cellType + "_" + column + "_" + str(index)
                    f.write("remove " + graphName + "\n")
                    f.write(filename +" setLabel "+graphName+" \n")
                    viewName = graphName + "_view"
                    f.write("remove "+viewName+" \n")
                    f.write("create HxLineRaycast " +viewName+" \n")
                    f.write(viewName + " data connect " + graphName + "\n")
                    f.write(viewName + " lineRadiusScale setValue 1.5 \n")
                    f.write(viewName + " display setState values 3 1 0 0 isTristate 0 0 0 mask 1 1 1 \n")
                    f.write(viewName + " lineConstColor setColor 0 " + colorR +" " + colorG +" " + colorB + "\n")
                    f.write(viewName + " compute \n")
                    index += 1

