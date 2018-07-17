# cortexinsilico Tools

This repository provides a collection of C++ tools to process anatomical models of the rat vibrissal cortex. 
The models are created with software tools developed at caesar institute Bonn and Zuse Institute Berlin. 

The tool *network simulator* is used in an ongoing research project to identify structure-based synapse formation rules using statistical inference.
For more detailed information about this DFG-funded project, refer to a [description](https://spp2041.de/85acc/) at the SPP 2041 Computational Connectomics website or [this](http://www.zib.de/projects/predicting-anatomically-realistic-cortical-connectomes-using-statistical-inference) project page at the ZIB website.

## How to compile all sources

Note that precompiled *releases* for Windows are made available under the releases tab. 
To compile all sources under linux, proceed as follows:

1) Navigate to the cortexinsilico directory

2) Run: 
```
python3 install/CompileCppAndRunTests.py
```
## How to use the network simulator

The *network simulator* tool provides two modes: SYNAPSE and SUBCUBE.

In the SYNAPSE mode, the tool computes synapse counts between neurons
according to generalized Peters' rule, which is parametrized by theta. To do so,
the tool requires neuron features that must be provided in the file *features.csv*
(in the same directory).

In the SUBCUBE mode, the tool creates a *features.csv* file by extracting a
subset of neurons from the complete model.

Usage:
```
./networkSimulator SYNAPSE <synapseSpecFile>
./networkSimulator SUBCUBE <voxelSpecFile>
```
- The *synapseSpecFile* contains the theta parameters for Peters' rule and the outmode mode. 
- The *voxelSpecFile* contains the model data directory and specifications of the neuron subset.

**For detailed instructions, refer to the PDF manual of the latest release (see *releases* tab on the github page).**

Note that in order to extract subsets from the model, you need to download the original model data from the ZIB website:

1) [Download the model data](https://visual.zib.de/2018/IXDtH2G8/)

2) Extract modelData into a desired folder.

3) Set this folder as DATA_ROOT in the *voxelSpecFile*.

4) Define the properties of the subvolume VOXEL_ORIGIN anf VOXEL_DIMENSIONS within the *voxelSpecFile*. 

5) Run the network simulator in the SUBCUBE mode.

## How to implement and run custom statistics 

1) [Download the precomputed innervation data](https://visual.zib.de/2018/IXDtH2G8/)

2) View class src/computeStatistic/MyStatistic.h as template for your own implementation.

3) Edit src/computeStatistic/mySpecFile.json to set the desired neuron filters.

- DATA_ROOT: points to the directory containing the innervation matrix (multiple files) and meta information about the network
- OUTPUT_DIR: ignore
- PRE_NEURON_REGIONS, PRE_NEURON_CELLTYPES, PRE_NEURON_IDS: filter definition (additive) for the presynaptic neurons that are part of the statistic
- POST_NEURON_REGIONS, POST_NEURON_CELLTYPES, POST_NEURON_IDS: filter definition (additive) for the postsynaptic neurons that are part of the statistic

The celltype and region identifiers are specified in the meta information files
(CellType.csv, Regions.csv) that are part of the raw data.

4) To test the computation of statistics locally, run the command: 
```
./build/computeStatistic/relase/computeStatistic <path_to_specFile>
```
