import sys
import os
import subprocess
import json
import csv

"""
This script calls the networkSimulator to create voxelwise synapse counts and can 
be used as starting point to integrate existing inference algorithms.
"""

specFile = "synapseSpec.json"
#simulator = "networkSimulator.exe"
simulator = "./networkSimulator"
featuresFile = "features.csv"
synapseFile = "synapses.csv"

# Load default spec file as template
with open(specFile) as f:
    spec = json.load(f)

# Set theta in tempoary spec file. Example with fixed theta [0,1,1,-1]:
spec["CONNECTIVITY_RULE_PARAMETERS"] = [0,1,1,-1]
newSpecFilePath = os.path.join(os.path.dirname(specFile), "generatedSpecFile.json")
with open(newSpecFilePath, 'w') as generatedSpecFile:
    json.dump(spec, generatedSpecFile)

# Call the simulator and remove temporary spec file.
subprocess.run([simulator,"SYNAPSE",newSpecFilePath])
os.remove(newSpecFilePath)

# Read in the synapses.csv file for further processing.
# In this example, we simply count all synapses. You have access to all fields specified the csv header.
print("Python script: counting all synapses in csv-file")
totalCount = 0
with open(synapseFile) as csvfile:
    reader = csv.DictReader(csvfile)
    for row in reader:
        totalCount += (int)(row["count"])
print("total number of synapses:",totalCount)

# Note that you can load the features.csv file in a similar way.
#
#with open(featuresFile) as csvfile:
#    reader = csv.DictReader(csvfile)
#    for row in reader:
#        print(row["voxelID"],row["neuronID"],row["morphologicalCellType"],row["pre"])
