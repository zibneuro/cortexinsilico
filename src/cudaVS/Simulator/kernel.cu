
#include "cuda_runtime.h"
#include "device_launch_parameters.h"

#include <stdio.h>
#include <map>
#include <set>
#include <vector>
#include <string>
#include <iostream>
#include <windows.h>
#include <fstream>
#include <ctime>
#include <random>

/*
#####################################################################################
READ FEATURES
#####################################################################################
*/

std::string getBaseDir() {
	char buffer[MAX_PATH];
	GetModuleFileName(NULL, buffer, MAX_PATH);
	std::string::size_type pos = std::string(buffer).find_last_of("\\/");
	return std::string(buffer).substr(0, pos);
}

std::string getDirPath(const std::string baseDir, const std::string dirName) {
	std::string path = baseDir;
	path.append("\\");
	path.append(dirName);
	return path;
}

std::vector<std::string> getFiles(const std::string& dir)
{
	std::vector<std::string> v;
	std::string pattern(dir);
	pattern.append("\\*.dat");
	WIN32_FIND_DATA data;
	HANDLE hFind;
	if ((hFind = FindFirstFile(pattern.c_str(), &data)) != INVALID_HANDLE_VALUE) {
		do {
			std::string path = dir;
			path.append("\\");
			path.append(data.cFileName);
			v.push_back(path);
		} while (FindNextFile(hFind, &data) != 0);
		FindClose(hFind);
	}
	return v;
}

void readMapFloat(const std::string filePath, std::map<int, float>& voxel_value, float coefficient) {
	std::ifstream infile(filePath);
	int voxelId;
	float value;
	while (infile >> voxelId >> value)
	{
		voxel_value[voxelId] = coefficient * value;
	}
}

int getIndexFromPath(const std::string path) {
	std::size_t pos1 = path.find_last_of("\\");
	std::string fileName = path.substr(pos1 + 1);
	return std::stoi(fileName);
}

void readFields(const std::vector<std::string>& filePaths, std::map<int, std::map<int, float> >& neuron_voxel_value, float coefficient) {
	for (auto it = filePaths.begin(); it != filePaths.end(); ++it) {
		int neuronId = getIndexFromPath(*it);
		std::map<int, float> voxel_value;
		readMapFloat(*it, voxel_value, coefficient);
		neuron_voxel_value[neuronId] = voxel_value;
	}
}

/*
#####################################################################################
PREPARE DATA
#####################################################################################
*/

void flattenFeatures(std::map<int, std::map<int, float> >& neuron_voxel_pre,
	std::map<int, std::map<int, float> >& neuron_voxel_postExc,
	std::map<int, float>& voxel_postAllExc,
	float* pre,
	float* post,
	float* postAll) {
	int i = 0;
	int nVoxel = voxel_postAllExc.size();
	for (auto it = voxel_postAllExc.begin(); it != voxel_postAllExc.end(); ++it) {
		int voxelId = it->first;
		int j = 0;
		for (auto it2 = neuron_voxel_pre.begin(); it2 != neuron_voxel_pre.end(); ++it2) {
			auto x = it2->second.find(voxelId);
			if (x != it2->second.end()) {
				pre[j * nVoxel + i] = x->second;
			}
			else {
				pre[j * nVoxel + i] = 0;
			}
			j++;
		}
		j = 0;
		for (auto it2 = neuron_voxel_postExc.begin(); it2 != neuron_voxel_postExc.end(); ++it2) {
			auto x = it2->second.find(voxelId);
			if (x != it2->second.end()) {
				post[j * nVoxel + i] = x->second;
			}
			else {
				post[j * nVoxel + i] = 0;
			}
			j++;
		}
		postAll[i] = it->second;
		i++;
	}
}

__global__ void addKernel(int *c, const int *a, const int *b)
{
	int i = threadIdx.x;
	c[i] = a[i] + b[i];
}

// Helper function for using CUDA to add vectors in parallel.
cudaError_t addWithCuda(int *c, const int *a, const int *b, unsigned int size)
{
	int *dev_a = 0;
	int *dev_b = 0;
	int *dev_c = 0;
	cudaError_t cudaStatus;

	// Choose which GPU to run on, change this on a multi-GPU system.
	cudaStatus = cudaSetDevice(0);
	if (cudaStatus != cudaSuccess) {
		fprintf(stderr, "cudaSetDevice failed!  Do you have a CUDA-capable GPU installed?");
		goto Error;
	}

	// Allocate GPU buffers for three vectors (two input, one output)    .
	cudaStatus = cudaMalloc((void**)&dev_c, size * sizeof(int));
	if (cudaStatus != cudaSuccess) {
		fprintf(stderr, "cudaMalloc failed!");
		goto Error;
	}

	cudaStatus = cudaMalloc((void**)&dev_a, size * sizeof(int));
	if (cudaStatus != cudaSuccess) {
		fprintf(stderr, "cudaMalloc failed!");
		goto Error;
	}

	cudaStatus = cudaMalloc((void**)&dev_b, size * sizeof(int));
	if (cudaStatus != cudaSuccess) {
		fprintf(stderr, "cudaMalloc failed!");
		goto Error;
	}

	// Copy input vectors from host memory to GPU buffers.
	cudaStatus = cudaMemcpy(dev_a, a, size * sizeof(int), cudaMemcpyHostToDevice);
	if (cudaStatus != cudaSuccess) {
		fprintf(stderr, "cudaMemcpy failed!");
		goto Error;
	}

	cudaStatus = cudaMemcpy(dev_b, b, size * sizeof(int), cudaMemcpyHostToDevice);
	if (cudaStatus != cudaSuccess) {
		fprintf(stderr, "cudaMemcpy failed!");
		goto Error;
	}

	// Launch a kernel on the GPU with one thread for each element.
	//addKernel << <1, size >> > (dev_c, dev_a, dev_b);

	// Check for any errors launching the kernel
	cudaStatus = cudaGetLastError();
	if (cudaStatus != cudaSuccess) {
		fprintf(stderr, "addKernel launch failed: %s\n", cudaGetErrorString(cudaStatus));
		goto Error;
	}

	// cudaDeviceSynchronize waits for the kernel to finish, and returns
	// any errors encountered during the launch.
	cudaStatus = cudaDeviceSynchronize();
	if (cudaStatus != cudaSuccess) {
		fprintf(stderr, "cudaDeviceSynchronize returned error code %d after launching addKernel!\n", cudaStatus);
		goto Error;
	}

	// Copy output vector from GPU buffer to host memory.
	cudaStatus = cudaMemcpy(c, dev_c, size * sizeof(int), cudaMemcpyDeviceToHost);
	if (cudaStatus != cudaSuccess) {
		fprintf(stderr, "cudaMemcpy failed!");
		goto Error;
	}

Error:
	cudaFree(dev_c);
	cudaFree(dev_a);
	cudaFree(dev_b);

	return cudaStatus;
}

/*
#####################################################################################
MAIN
#####################################################################################
*/

int main(int argc, char *argv[]) {

	/*
	if (argc != 5) {
		std::cout << "Usage:" << std::endl;
		std::cout << "Simulator.exe theta1 theta2 theta3 theta4" << std::endl;
		return -1;
	}*/

	float b0, b1, b2, b3;
	b0 = 0;//std::stof(argv[1]);
	b1 = 1;// std::stof(argv[2]);
	b2 = 1;// std::stof(argv[3]);
	b3 = -1;// std::stof(argv[4]);

	std::cout << "Start simulation" << " " << b0 << " " << b1 << " " << b2 << " " << b3 << std::endl;
	std::clock_t start;
	double duration;
	start = std::clock();

	std::map<int, std::map<int, float> > neuron_voxel_pre;
	std::map<int, std::map<int, float> > neuron_voxel_postExc;
	std::map<int, float> voxel_postAllExc;

	std::string baseDir = getBaseDir();

	duration = (std::clock() - start) / (double)CLOCKS_PER_SEC;
	std::cout << "Start list files pre" << " " << duration << std::endl;

	std::vector<std::string> preFiles = getFiles(getDirPath(baseDir, "features_pre"));

	duration = (std::clock() - start) / (double)CLOCKS_PER_SEC;
	std::cout << "Listed files pre" << " " << duration << std::endl;

	readFields(preFiles, neuron_voxel_pre, b1);

	duration = (std::clock() - start) / (double)CLOCKS_PER_SEC;
	std::cout << "Read features pre" << " " << duration << std::endl;
	
	readFields(getFiles(getDirPath(baseDir, "features_postExc")), neuron_voxel_postExc, b2);

	duration = (std::clock() - start) / (double)CLOCKS_PER_SEC;
	std::cout << "Read features post" << " " << duration << std::endl;

	readMapFloat(getDirPath(baseDir, "features_postAll").append("\\voxel_postAllExc.dat"), voxel_postAllExc, b3);

	duration = (std::clock() - start) / (double)CLOCKS_PER_SEC;
	std::cout << "Read data" << " " << duration << std::endl;

	std::size_t nPre = neuron_voxel_pre.size();
	std::size_t nPost = neuron_voxel_postExc.size();
	std::size_t nVoxel = voxel_postAllExc.size();

	float* pre = (float*)malloc(nPre * nVoxel * sizeof(float));
	float* post = (float*)malloc(nPost * nVoxel * sizeof(float));
	float* postAll = (float*)malloc(nVoxel * sizeof(float));

	flattenFeatures(neuron_voxel_pre, neuron_voxel_postExc, voxel_postAllExc, pre, post, postAll);

	duration = (std::clock() - start) / (double)CLOCKS_PER_SEC;
	std::cout << "Flattened data" << " " << duration << std::endl;

	std::random_device rd;
	std::mt19937 randomGenerator(rd());

	std::vector<int> empty(nPost, 0);
	std::vector<std::vector<int> > contacts(nPre, empty);
	std::uniform_real_distribution<float> dis(0.0, 1.0);

	#pragma omp parallel for schedule(dynamic)
	for (unsigned int i = 0; i < nPre; i++)
	{
		for (unsigned int j = 0; j < nPost; j++)
		{
			for (unsigned int k = 0; k < nVoxel; k++)
			{
				float preVal = pre[i * nVoxel + k];
				float postVal = post[j * nVoxel + k];
				float postAllVal = postAll[k];
				if (preVal != 0 && postVal != 0) {
					float arg = b0 + preVal + postVal + postAllVal;
					int synapses = 0;
					if (arg >= -7 && arg <= 7)
					{
						float mu = exp(arg);
						float prob = 1 - exp(-1 * mu);
						float rand = dis(randomGenerator);
						synapses = rand <= prob ? 1 : 0;
					}
					else if (arg > 7)
					{
						synapses = 1;
					}
					if (synapses > 0)
					{
						contacts[i][j] = synapses;
						break;
					}
				}
			}
		}
	}

	duration = (std::clock() - start) / (double)CLOCKS_PER_SEC;
	std::cout << "Finished synapse distribution" << " " << duration << std::endl;

	float connectionProbability = 0;

	for (unsigned int i = 0; i < nPre; i++) {
		int realizedConnections = 0;
		for (unsigned int j = 0; j < nPost; j++) {
			realizedConnections += contacts[i][j] > 0 ? 1 : 0;
		}
		connectionProbability += (float)realizedConnections / (float)nPost;
	}
	connectionProbability /= (float)nPre;

	duration = (std::clock() - start) / (double)CLOCKS_PER_SEC;
	std::cout << "Computed connection probability" << " " << duration << std::endl;

	/*
	cudaError_t cudaStatus = addWithCuda(c, a, b, arraySize);
	if (cudaStatus != cudaSuccess) {
		fprintf(stderr, "addWithCuda failed!");
		return 1;
	*/

	/*
	cudaError_t cudaStatus = cudaDeviceReset();
	if (cudaStatus != cudaSuccess) {
		fprintf(stderr, "cudaDeviceReset failed!");
		return 1;
	}
	*/

	std::cout << "Finish simulation" << " " << b0 << " " << b1 << " " << b2 << " " << b3 << " prob. " << connectionProbability << std::endl;

	free(pre);
	free(post);
	free(postAll);

	return 0;
}




