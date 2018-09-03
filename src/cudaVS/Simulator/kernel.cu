
#include "cuda_runtime.h"
#include "device_launch_parameters.h"

#include <QChar>
#include <QDebug>
#include <QFile>
#include <QIODevice>
#include <QList>
#include <QString>
#include <QStringList>
#include <QTextStream>
#include <QDirIterator>

#include <stdio.h>
#include <map>
#include <set>

cudaError_t addWithCuda(int *c, const int *a, const int *b, unsigned int size);

__global__ void addKernel(int *c, const int *a, const int *b)
{
    int i = threadIdx.x;
    c[i] = a[i] + b[i];
}

void
readMapFloat(std::map<int, float>& mapping, QString folder, QString fileName, float coefficient)
{
	if (folder != "")
	{
		fileName = QDir(folder).filePath(fileName);
	}
	QFile file(fileName);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		const QString msg =
			QString("Error reading features file. Could not open file %1").arg(fileName);
		throw std::runtime_error(qPrintable(msg));
	}

	QTextStream in(&file);
	QString line = in.readLine();
	while (!line.isNull())
	{
		QStringList parts = line.split(' ');
		mapping[parts[0].toInt()] = coefficient * parts[1].toFloat();
		line = in.readLine();
	}
}

void
readMapInt(std::map<int, int>& mapping, QString folder, QString fileName)
{
	if (folder != "")
	{
		fileName = QDir(folder).filePath(fileName);
	}
	QFile file(fileName);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		const QString msg =
			QString("Error reading features file. Could not open file %1").arg(fileName);
		throw std::runtime_error(qPrintable(msg));
	}

	QTextStream in(&file);
	QString line = in.readLine();
	while (!line.isNull())
	{
		QStringList parts = line.split(' ');
		mapping[parts[0].toInt()] = parts[1].toInt();
		line = in.readLine();
	}
}


void
load(std::map<int, std::map<int, float> >& neuron_pre,
	float b1,
	std::map<int, std::map<int, float> >& neuron_postExc,
	float b2,
	std::map<int, std::map<int, float> >& neuron_postInh,
	std::map<int, float>& voxel_postAllExc,
	float b3,
	std::map<int, float>& voxel_postAllInh,
	std::map<int, int>& /*neuron_funct*/,
	std::map<int, std::set<int> >& /*voxel_neuronsPre*/,
	std::map<int, std::set<int> >& /*voxel_neuronsPostExc*/,
	std::map<int, std::set<int> >& /*voxel_neuronsPostInh*/)
{
	QDirIterator it_pre("features_pre");
	while (it_pre.hasNext())
	{
		QString file = it_pre.next();
		if (file.contains(".dat"))
		{
			QFileInfo fileInfo(file);
			int neuron = fileInfo.baseName().toInt();
			std::map<int, float> foo;
			readMapFloat(foo, "", file, b1);
			neuron_pre[neuron] = foo;
		}
	}
	//qDebug() << neuron_pre.size();

	QDirIterator it_postExc("features_postExc");
	while (it_postExc.hasNext())
	{
		QString file = it_postExc.next();
		if (file.contains(".dat"))
		{
			QFileInfo fileInfo(file);
			int neuron = fileInfo.baseName().toInt();
			std::map<int, float> foo;
			readMapFloat(foo, "", file, b2);
			neuron_postExc[neuron] = foo;
		}
	}

	QDirIterator it_postInh("features_postInh");
	while (it_postInh.hasNext())
	{
		QString file = it_postInh.next();
		if (file.contains(".dat"))
		{
			QFileInfo fileInfo(file);
			int neuron = fileInfo.baseName().toInt();
			std::map<int, float> foo;
			readMapFloat(foo, "", file,1);
			neuron_postInh[neuron] = foo;
		}
	}

	readMapFloat(voxel_postAllExc, "features_postAll", "voxel_postAllExc.dat", b3);
	readMapFloat(voxel_postAllInh, "features_postAll", "voxel_postAllInh.dat",1);
	/*
	readMapInt(neuron_funct, "features_meta", "neuron_funct.dat");

	readIndex(voxel_neuronsPre, "features_meta","voxel_neuronsPre.dat");
	readIndex(voxel_neuronsPostExc, "features_meta","voxel_neuronsPostExc.dat");
	readIndex(voxel_neuronsPostInh, "features_meta","voxel_neuronsPostInh.dat");
*/
}

/*
int main()
{
	fprintf(stderr, "addWithCuda failed!");
    const int arraySize = 5;
    const int a[arraySize] = { 1, 2, 3, 4, 5 };
    const int b[arraySize] = { 10, 20, 30, 40, 50 };
    int c[arraySize] = { 0 };

    // Add vectors in parallel.
    cudaError_t cudaStatus = addWithCuda(c, a, b, arraySize);
    if (cudaStatus != cudaSuccess) {
        fprintf(stderr, "addWithCuda failed!");
        return 1;
    }

    printf("{1,2,3,4,5} + {10,20,30,40,50} = {%d,%d,%d,%d,%d}\n",
        c[0], c[1], c[2], c[3], c[4]);

    // cudaDeviceReset must be called before exiting in order for profiling and
    // tracing tools such as Nsight and Visual Profiler to show complete traces.
    cudaStatus = cudaDeviceReset();
    if (cudaStatus != cudaSuccess) {
        fprintf(stderr, "cudaDeviceReset failed!");
        return 1;
    }

    return 0;
}
*/


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
    addKernel<<<1, size>>>(dev_c, dev_a, dev_b);

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
