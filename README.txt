I) How to compile all sources
=============================

1) From the cortexinsilico directory, run:
python3 install/CompileCppAndRunTests.py


II) How use the network simulator (to find connectivity rules)
==============================================================

1) Download the sampled model data from the ZIB website:

https://visual.zib.de/2018/IXDtH2G8/latest/

Factor 10 downsampled model:
realization-entireRBCF10_2018-04-19/modelData_entireRBCF10_2018-04-19.tar.gz

Entire model:
realization-entireRBC_2015-10-07/modelData_entireRBC_2015-10-07.tar.gz

2) Extract modelData_entireRBCF10_2018-04-19.tar.gz into:

data/

3) View and/or edit the src/networkSimulator/findRules.py script to integrate
existing Bayesian inference algorithms to sample the theta-parameter space.

4) View and/or edit the data/sampleNetworkSimulatorSpecFiles/spec.json to set
model-data filters (on which neurons to incorporate) and to define summary statistics.

5) From the cortexinsilico directory, run:
python3 src/networkSimulator/findRules.py data/sampleNetworkSimulatorSpecFiles/spec.json build/networkSimulator/release/networkSimulator

[6) View the generated summaryStatistics.json file in /data/connectome/]


III) How implement and run custom statistics (extending the library)
====================================================================

1) View class src/computeStatistic/MyStatistic.h as template for your own implementation.

2) Edit src/computeStatistic/mySpecFile.json to set the desired neuron filters.

- DATA_ROOT: points to the directory containing the innervation matrix (multiple files) and meta information about the network
- OUTPUT_DIR: ignore
- PRE_NEURON_REGIONS, PRE_NEURON_CELLTYPES, PRE_NEURON_IDS: filter definition (additive) for the presynaptic neurons that are part of the statistic
- POST_NEURON_REGIONS, POST_NEURON_CELLTYPES, POST_NEURON_IDS: filter definition (additive) for the postsynaptic neurons that are part of the statistic

The celltype and region identifiers are specified in the meta information files
(CellType.csv, Regions.csv) that are part of the raw data.

3) Run the command:

./build/computeStatistic/relase/computeStatistic <path_specFile>
