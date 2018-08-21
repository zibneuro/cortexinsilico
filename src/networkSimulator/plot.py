import numpy as np
import matplotlib.pylab as plt
import os
import sys

with open(sys.argv[1]) as f:
    data = f.read()

data = data.split('\n')
pre = len(data)
post = len(data[0].split(' '))

matrix = np.ndarray(shape=(pre,post))

for i in range(0,pre):
    vals = data[i].split(' ')
    for j in range(0,post):
        matrix[i,j]=int(vals[j])

print(pre)
print(post)

fig = plt.figure()
ax = fig.add_subplot(1,1,1)
ax.set_aspect('equal')
plt.imshow(matrix, interpolation='none', cmap=plt.cm.hot)
plt.colorbar()
plt.show()