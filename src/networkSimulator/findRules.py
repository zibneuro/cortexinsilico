"""
This script calls the networkSimulator to create voxelwise synapse counts and 
can be used as starting point to integrate existing inference algorithms.
"""

################################################################################
### LOAD LIBRARIES
################################################################################

import os
import json
import subprocess

import numpy as np
import pandas as pd
import statsmodels.api as sm

# import sys
# import csv

################################################################################
### SET FILE NAMES
################################################################################

specFile = "synapseSpec.json"
simulator = "networkSimulator.exe"
featuresFile = "features.csv"
synapseFile = "synapses.csv"

################################################################################
### GENERATE RULE-BASED SYNAPSE COUNTS
################################################################################

# Load default spec file as template
with open(specFile) as f:
    spec = json.load(f)

# Set theta in temporary spec file. Example with fixed theta [0, 1, 1, -1]:
true_beta = [0, 1, 1, -1]
spec["CONNECTIVITY_RULE_PARAMETERS"] = true_beta
newSpecFilePath = os.path.join(os.path.dirname(specFile),
                               "generatedSpecFile.json")
with open(newSpecFilePath, 'w') as generatedSpecFile:
    json.dump(spec, generatedSpecFile)

# Call the simulator and remove temporary spec file.
subprocess.run([simulator, "SYNAPSE", newSpecFilePath])
os.remove(newSpecFilePath)

################################################################################
### MAXIMUM LIKELIHOOD ESTIMATION (MLE)
################################################################################

# Read data
data = pd.read_csv("synapses.csv")
X = sm.add_constant(np.log(data.iloc[:, -len(true_beta):-1]))
X.columns = ["intercept", "boutons", "PSTs", "PSTs_all"]

# Estimate parameters
family = sm.families.Poisson(sm.families.links.log)
model = sm.GLM(data["count"], X, family=family)
model_fit = model.fit()

################################################################################
### PRINT RESULTS
################################################################################

print()
print(pd.DataFrame({"True": true_beta, "MLE": model_fit.params}))

"""
# Read in the synapses.csv file for further processing.
# In this example, we simply count all synapses. 
# You have access to all fields specified the csv header.
print("Python script: counting all synapses in csv-file")
totalCount = 0
with open(synapseFile) as csvfile:
    reader = csv.DictReader(csvfile)
    for row in reader:
        totalCount += (int)(row["count"])
print("total number of synapses:", totalCount)

# Note that you can load the features.csv file in a similar way.
with open(featuresFile) as csvfile:
    reader = csv.DictReader(csvfile)
    for row in reader:
        print(row["voxelID"], row["neuronID"], 
              row["morphologicalCellType"], row["pre"])
"""
