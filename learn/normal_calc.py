# example to calculate normal vector from 4 points
import numpy as np

def normalize(v):
	norm = np.linalg.norm(v)
	return v / norm if norm != 0 else v

p0 = np.array([0, 1/2, 0])
p1 = np.array([1, 1/2, 0])
p2 = np.array([1/2, 0, 0])
p3 = np.array([1/2, 1, 0])

n = normalize(np.cross(p1 - p0, p3 - p2))
print(f'n={n}')  #= n=[0. 0. 1.]
