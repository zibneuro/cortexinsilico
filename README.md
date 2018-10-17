# cortexinsilico Tools

This repository provides a collection of C++ tools to process anatomical models of the rat vibrissal cortex. 
The models have been created with software tools developed at caesar institute Bonn and Zuse Institute Berlin. 

The tool *network simulator* is used in an ongoing research project to identify structure-based synapse formation rules using statistical inference.
For more detailed information about this project, refer to a [description](https://spp2041.de/85acc/) at the SPP 2041 Computational Connectomics website or [this](http://www.zib.de/projects/predicting-anatomically-realistic-cortical-connectomes-using-statistical-inference) project page at the ZIB website.

**For project members:** We share our current work in a [private repository](https://github.com/fnyanez/rule_estimation). 

## How to compile all sources

For Windows, **precompiled releases** of the simulator are available under the [releases tab](https://github.com/zibneuro/cortexinsilico/releases). 

Under linux, the relevant sources can be compiled as follows:

1) Navigate to the cortexinsilico directory

2) Run: 
```
python3 install/CompileNetworkSimulatorUnix.py
or
python3 install/CompileCppMinimalSuse.py
```

## How to use the network simulator

**The best resource to get started are our current jupyter notebooks in [this repository](https://github.com/fnyanez/rule_estimation).**

Note that in order to work with the simulator, you need to download the original model data from the ZIB website:

1) Go to the [download page](https://visual.zib.de/2018/IXDtH2G8/) (Password on request).

2) Navigate to *realization-entireRBC_2015-10-07*.

3) Download *modelData_entireRBC_2015-10-07.tar.gz*.

4) Extract the downloaded folder: On Windows, run [7Zip](http://www.7-zip.de/) twice. Under Linux, run:
```
tar -xvzf modelData_entireRBC_2015-10-07.tar.gz 
```
5) Set the extracted folder as *data_root* in the jupyter notebook.
