
#include "cuda_runtime.h"
#include "device_launch_parameters.h"
#include <curand_kernel.h>

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
#include <omp.h>
#include <fstream>

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

/*
#####################################################################################
GPU COMPUTATION
#####################################################################################
*/

__global__ void calcKernel(float *contacts,
	float *pre,
	float *post,
	float* postAll,
	float b0,
	unsigned int nVoxel,
	unsigned int nPre,
	unsigned int nPost
)
{
	int i = blockIdx.x * blockDim.x + threadIdx.x;
	int j = blockIdx.y * blockDim.y + threadIdx.y;

	if (i < nPre && j < nPost) {
		for (unsigned int k = 0; k < nVoxel; k++)
		{
			float preVal = pre[i * nVoxel + k];
			float postVal = post[j * nVoxel + k];
			if (preVal != 0 && postVal != 0 && contacts[i * nPost + j] < 1000) {
				float arg = b0 + preVal + postVal + postAll[k];
				int synapses = 0;
				if (arg >= -7 && arg <= 7)
				{
					float mu = exp(arg);
					contacts[i * nPost + j] += mu;
				}
				else if (arg > 7)
				{
					contacts[i * nPost + j] = 1000;
				}
			}
		}
	}
}

// Helper function for using CUDA to add vectors in parallel.
cudaError_t calcWithCuda(float *pre,
	float *post,
	float *postAll,
	float b0,
	unsigned int nPre,
	unsigned int nPost,
	unsigned int nVoxel,
	float* contacts,
	std::clock_t start,
	bool verbose)
{
	float *dev_pre = 0;
	float *dev_post = 0;
	float *dev_postAll = 0;
	float *dev_contacts = 0;
	cudaError_t cudaStatus;

	double copyToDeviceStartTime = std::clock();

	// Choose which GPU to run on, change this on a multi-GPU system.
	cudaStatus = cudaSetDevice(0);
	if (cudaStatus != cudaSuccess) {
		fprintf(stderr, "cudaSetDevice failed!  Do you have a CUDA-capable GPU installed?");
		goto Error;
	}

	cudaStatus = cudaMalloc((void**)&dev_pre, nPre * nVoxel * sizeof(float));
	if (cudaStatus != cudaSuccess) {
		fprintf(stderr, "cudaMalloc failed!");
		goto Error;
	}

	cudaStatus = cudaMalloc((void**)&dev_post, nPost * nVoxel * sizeof(float));
	if (cudaStatus != cudaSuccess) {
		fprintf(stderr, "cudaMalloc failed!");
		goto Error;
	}

	cudaStatus = cudaMalloc((void**)&dev_postAll, nVoxel * sizeof(float));
	if (cudaStatus != cudaSuccess) {
		fprintf(stderr, "cudaMalloc failed!");
		goto Error;
	}

	cudaStatus = cudaMalloc((void**)&dev_contacts, nPre * nPost * sizeof(float));
	if (cudaStatus != cudaSuccess) {
		fprintf(stderr, "cudaMalloc failed!");
		goto Error;
	}

	cudaStatus = cudaMemcpy(dev_pre, pre, nPre * nVoxel * sizeof(float), cudaMemcpyHostToDevice);
	if (cudaStatus != cudaSuccess) {
		fprintf(stderr, "cudaMemcpy failed!");
		goto Error;
	}

	cudaStatus = cudaMemcpy(dev_post, post, nPost * nVoxel * sizeof(float), cudaMemcpyHostToDevice);
	if (cudaStatus != cudaSuccess) {
		fprintf(stderr, "cudaMemcpy failed!");
		goto Error;
	}

	cudaStatus = cudaMemcpy(dev_postAll, postAll, nVoxel * sizeof(float), cudaMemcpyHostToDevice);
	if (cudaStatus != cudaSuccess) {
		fprintf(stderr, "cudaMemcpy failed!");
		goto Error;
	}

	cudaStatus = cudaMemcpy(dev_contacts, contacts, nPre * nPost * sizeof(bool), cudaMemcpyHostToDevice);
	if (cudaStatus != cudaSuccess) {
		fprintf(stderr, "cudaMemcpy failed!");
		goto Error;
	}

	double duration;
	if (verbose) {
		duration = (std::clock() - copyToDeviceStartTime) / (double)CLOCKS_PER_SEC;
		std::cout << "[*] Copy to device " << duration << std::endl;
	}
	double computeKernelStartTime = std::clock();

	// Launch a kernel on the GPU
	dim3 threads(16, 16);
	dim3 blocks(nPre / threads.x + 1, nPost / threads.y + 1);
	calcKernel << <blocks, threads >> > (dev_contacts,
		dev_pre,
		dev_post,
		dev_postAll,
		b0,
		nVoxel,
		nPre,
		nPost
		);

	// Check for any errors launching the kernel
	cudaStatus = cudaGetLastError();
	if (cudaStatus != cudaSuccess) {
		fprintf(stderr, "calcKernel launch failed: %s\n", cudaGetErrorString(cudaStatus));
		goto Error;
	}

	// cudaDeviceSynchronize waits for the kernel to finish, and returns
	// any errors encountered during the launch.
	cudaStatus = cudaDeviceSynchronize();
	if (cudaStatus != cudaSuccess) {
		fprintf(stderr, "cudaDeviceSynchronize returned error code %d after launching addKernel!\n", cudaStatus);
		goto Error;
	}

	if (verbose) {
		duration = (std::clock() - computeKernelStartTime) / (double)CLOCKS_PER_SEC;
		std::cout << "[*] Compute GPU " << duration << std::endl;
	}
	double copyToHostTime = std::clock();

	// Copy output vector from GPU buffer to host memory.
	cudaStatus = cudaMemcpy(contacts, dev_contacts, nPre * nPost * sizeof(float), cudaMemcpyDeviceToHost);
	if (cudaStatus != cudaSuccess) {
		fprintf(stderr, "cudaMemcpy failed!");
		goto Error;
	}

	if (verbose) {
		duration = (std::clock() - copyToHostTime) / (double)CLOCKS_PER_SEC;
		std::cout << "[*] Copy to host " << duration << std::endl;
	}
Error:
	cudaFree(dev_pre);
	cudaFree(dev_post);
	cudaFree(dev_postAll);
	cudaFree(dev_contacts);

	return cudaStatus;
}

/*
#####################################################################################
MAIN
#####################################################################################
*/

int main(int argc, char *argv[]) {

	if (argc != 6) {
		std::cout << "Usage:" << std::endl;
		std::cout << "Simulator.exe CPU|GPU theta1 theta2 theta3 theta4" << std::endl;
		return -1;
	}

	std::string mode = argv[1];
	bool gpu = mode.compare("GPU") == 0;

	float b0, b1, b2, b3;
	b0 = std::stof(argv[2]);
	b1 = std::stof(argv[3]);
	b2 = std::stof(argv[4]);
	b3 = std::stof(argv[5]);

	bool verbose = false;

	std::cout << "[*] Start simulation " << b0 << " " << b1 << " " << b2 << " " << b3 << std::endl;
	std::clock_t start;
	double duration;
	start = std::clock();

	std::map<int, std::map<int, float> > neuron_voxel_pre;
	std::map<int, std::map<int, float> > neuron_voxel_postExc;
	std::map<int, float> voxel_postAllExc;

	std::string baseDir = getBaseDir();

	double readFeatureTime = std::clock();

	std::vector<std::string> preFiles = getFiles(getDirPath(baseDir, "features_pre"));
	readFields(preFiles, neuron_voxel_pre, b1);
	readFields(getFiles(getDirPath(baseDir, "features_postExc")), neuron_voxel_postExc, b2);
	readMapFloat(getDirPath(baseDir, "features_postAll").append("\\voxel_postAllExc.dat"), voxel_postAllExc, b3);

	if (verbose) {
		duration = (std::clock() - readFeatureTime) / (double)CLOCKS_PER_SEC;
		std::cout << "[*] Read features " << duration << std::endl;
	}
	std::size_t nPre = neuron_voxel_pre.size();
	std::size_t nPost = neuron_voxel_postExc.size();
	std::size_t nVoxel = voxel_postAllExc.size();

	if (verbose) {
		std::cout << "[*] Presynaptic: " << nPre << " Postsynaptic: " << nPost << " Voxels: " << nVoxel << std::endl;
	}
	std::random_device rd;
	std::mt19937 randomGenerator(rd());
	std::uniform_real_distribution<float> dis(0.0, 1.0);

	float* connections = (float*)malloc(nPre * nPost * sizeof(float));

	for (int i = 0; i < nPre; i++) {
		for (int j = 0; j < nPost; j++) {
			connections[i * nPost + j] = 0;
		}
	}

	if (!gpu) {

		std::vector<int> preIndices;
		std::vector<int> postIndices;

		for (auto it = neuron_voxel_pre.begin(); it != neuron_voxel_pre.end(); ++it)
		{
			preIndices.push_back(it->first);
		}

		for (auto it = neuron_voxel_postExc.begin(); it != neuron_voxel_postExc.end(); ++it)
		{
			postIndices.push_back(it->first);
		}

		double computeCPUStartTime = std::clock();

#pragma omp parallel for schedule(dynamic)
		for (unsigned int i = 0; i < preIndices.size(); i++)
		{
			int preId = preIndices[i];
			for (unsigned int j = 0; j < postIndices.size(); j++)
			{
				int postId = postIndices[j];
				//qDebug() << i << j << preId << postId;
				if (preId != postId)
				{
					for (auto pre = neuron_voxel_pre[preId].begin(); pre != neuron_voxel_pre[preId].end(); ++pre)
					{
						if (neuron_voxel_postExc[postId].find(pre->first) != neuron_voxel_postExc[postId].end())
						{
							float preVal = pre->second;
							float postVal = neuron_voxel_postExc[postId][pre->first];
							float postAllVal = voxel_postAllExc[pre->first];
							float arg = b0 + preVal + postVal + postAllVal;
							//int synapses = 0;
							if (arg >= -7 && arg <= 7)
							{
								float mu = exp(arg);
								connections[i * nPost + j] += mu;
							}
							else if (arg > 7)
							{
								connections[i * nPost + j] = 1000;
								break;
							}
						}
					}
				}
			}
		}

		if (verbose) {
			duration = (std::clock() - computeCPUStartTime) / (double)CLOCKS_PER_SEC;
			std::cout << "[*] Compute CPU " << duration << std::endl;
		}
	}
	else
	{
		float* pre = (float*)malloc(nPre * nVoxel * sizeof(float));
		float* post = (float*)malloc(nPost * nVoxel * sizeof(float));
		float* postAll = (float*)malloc(nVoxel * sizeof(float));

		double flattenFeaturesTime = std::clock();

		flattenFeatures(neuron_voxel_pre, neuron_voxel_postExc, voxel_postAllExc, pre, post, postAll);

		if (verbose) {
			duration = (std::clock() - flattenFeaturesTime) / (double)CLOCKS_PER_SEC;
			std::cout << "[*] Flatten features " << duration << std::endl;
		}
		cudaError_t cudaStatus = calcWithCuda(pre, post, postAll, b0, nPre, nPost, nVoxel, connections, start, verbose);

		if (cudaStatus != cudaSuccess) {
			fprintf(stderr, "calcWithCuda failed!");
			return 1;
		}

		cudaStatus = cudaDeviceReset();
		if (cudaStatus != cudaSuccess) {
			fprintf(stderr, "cudaDeviceReset failed!");
			return 1;
		}

		free(pre);
		free(post);
		free(postAll);
	}

	double computeProbabilityStartTime = std::clock();

	float connectionProbability = 0;
	for (unsigned int i = 0; i < nPre; i++) {
		int realizedConnections = 0;
		for (unsigned int j = 0; j < nPost; j++) {
			float mu = connections[i * nPost + j];
			if (mu <= 1000) {
				float prob = 1 - exp(-1 * mu);
				float rand = dis(randomGenerator);
				realizedConnections += rand <= prob ? 1 : 0;
			}
			else {
				realizedConnections++;
			}
		}
		connectionProbability += (float)realizedConnections / (float)nPost;
	}
	connectionProbability /= (float)nPre;
	free(connections);

	duration = (std::clock() - computeProbabilityStartTime) / (double)CLOCKS_PER_SEC;
	//std::cout << "[*] Compute connection probability " << duration << std::endl;

	std::ofstream outfile;
	outfile.open("output.json");
	outfile << "{\"CONNECTION_PROBABILITY\":" << connectionProbability << "}";
	outfile.close();

	duration = (std::clock() - start) / (double)CLOCKS_PER_SEC;
	std::cout << "[*] Finish simulation " << duration << "s" << std::endl;
	std::cout << "[*] Connection prob. " << connectionProbability << std::endl;


	return 0;
}




