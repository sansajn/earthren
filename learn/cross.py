# calculate cross product from two vectors u and v
import numpy as np

u = np.array([0.86, 0.14, 0.49])
v = np.array([2,3,4])

s = np.cross(u, v)
print(f'cross({u}, {v})={s}')
