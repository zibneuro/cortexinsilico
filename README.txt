To compile all sources:

python3 install/CompileCppAndRunTests.py


To compute a custom statistic:

0) View template class src/computeStatistic/MyStatistic.h to get started with your own implementation.

1) Edit src/computeStatistic/mySpecFile.json

- DATA_ROOT: points to the directory containing the innervation matrix (multiple files) and meta information about the network
- OUTPUT_DIR: ignore
- PRE_NEURON_REGIONS, PRE_NEURON_CELLTYPES, PRE_NEURON_IDS: filter definition (additive) for the presynaptic neurons that are part of the statistic
- POST_NEURON_REGIONS, POST_NEURON_CELLTYPES, POST_NEURON_IDS: filter definition (additive) for the postsynaptic neurons that are part of the statistic

The celltype and region identifiers are specified in the meta information files that are part of the raw data.

2) Run the script:

./build/computeStatistic/relase/computeStatistic <path_specFile>




