import numpy as np
import matplotlib.pylab as plt
import os
import sys

index = 1
maxRange = 30

files = []
imageNames = []

for filename in os.listdir(os.getcwd()):
    files.append(filename)
    imageNames.append("{0:0>5}".format(index))        
    index += 1

for k in range(0,100):

    with open(files[k]) as f:
        data = f.read()
    print(imageNames[k],files[k])
    data = data.split('\n')
    pre = len(data)
    post = len(data[0].split(' '))

    matrix = np.ndarray(shape=(pre,post))     

    for i in range(0,pre):
        vals = data[i].split(' ')
        for j in range(0,post):
            matrix[i,j]=int(vals[j])

    maxRange = np.amax(matrix)   

    fig = plt.figure(figsize=(8, 6), dpi=200)
    ax = fig.add_subplot(1,1,1)
    ax.set_aspect('equal')
    plt.imshow(matrix, interpolation='none', cmap=plt.cm.winter, vmin=0, vmax=maxRange)
    plt.colorbar()
    plt.title(files[k])
    plt.xlabel('post neuron')
    plt.ylabel('pre neuron')
    plt.savefig(os.path.join(imageNames[k]+'.png'))
    plt.close()