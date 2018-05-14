import sys
import os
import subprocess
import json

"""
Calls the networkSimulator iteratively with varying theta-values to generate
summary statistics. Reads the generated summary statistics from file
and feeds the statistics into the inference algorithm to retrieve new theta-values.
"""

def printUsageAndExit():
    print("Usage:")
    print("python3 findRules.py <spec-file> <networkSimulator>")
    print()
    print("<spec-file> = The spec file containing the summary statistic definitions and IO-paths.")
    print("<networkSimulator> = path to the networkSimulator executable.")
    sys.exit(1)

if (len(sys.argv) != 3):
    printUsageAndExit()

specFile = sys.argv[1]
simulator = sys.argv[2]

with open(specFile) as f:
    spec = json.load(f)

# Generate samples with varying theta
for i in range(1):

    # Set theta. Example with fixed theta [0,1,1,-1]:
    spec["CONNECTIVITY_RULE_PARAMETERS"] = [0,1,1,-1]
    newSpecFilePath = os.path.join(os.path.dirname(specFile), "generatedSpecFile.json")
    with open(newSpecFilePath, 'w') as generatedSpecFile:
        json.dump(spec, generatedSpecFile)

    # Call the simulator
    subprocess.run([simulator,newSpecFilePath])

    # Extract the generated summary statistics
    summaryStatisticFile = os.path.join(spec["OUTPUT_DIR"],"summaryStatistics.json")
    with open(summaryStatisticFile) as f:
        summaryStatistics = json.load(f)

    statistics = summaryStatistics["SUMMARY_STATISTICS"]
    for j in range(len(statistics)):
        stat = statistics[j]
        if "OK" in stat["STATUS"]:
            print(stat["STATISTIC_NAME"],stat["RESULT"])

    # Feed the statistics into the Bayesian inference framework
    # ... and set next theta.
