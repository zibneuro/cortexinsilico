To compile the demo script:

cd /demo
qmake
make

Edit mySpecFile.json

DATA_ROOT: points to the directory containing the innervation matrix (multiple files) and meta information about the network
OUTPUT_DIR: ignore
PRE_NEURON_REGIONS, PRE_NEURON_CELLTYPES, PRE_NEURON_IDS: filter definition (or/additive) for the presynaptic neurons that are part of the statistic
POST_NEURON_REGIONS, POST_NEURON_CELLTYPES, POST_NEURON_IDS: filter definition (or/additive) for the postsynaptic neurons that are part of the statistic

The celltype and region identifiers are specified in the meta information files that are part of the raw data.


To run the scrip:

release/computeStatistic mySpecFile.json




