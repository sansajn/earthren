# how to constructs a view transformation matrix from a local coordinate system and a position
import numpy as np

def create_view_matrix(cx, cy, cz, pos):
	R = np.array([  # Rotation matrix part
		[cx[0], cx[1], cx[2], 0],
		[cy[0], cy[1], cy[2], 0],
		[cz[0], cz[1], cz[2], 0],
		[0, 0, 0, 1]
	])
    
	T = np.array([  # Translation matrix part
		[1, 0, 0, -pos[0]],
		[0, 1, 0, -pos[1]],
		[0, 0, 1, -pos[2]],
		[0, 0, 0, 1]
	])
	
	V = R @ T  # View matrix is the product of the rotation and translation matrices
    
	return V

cx = np.array([1, 0, 0])  # Right vector
cy = np.array([0, 1, 0])  # Up vector
cz = np.array([0, 0, 1])  # Forward vector (looking towards -z in camera space)
pos = np.array([10, 5, 20])  # Camera position in world space

view = create_view_matrix(cx, cy, cz, pos)
print(f'view=\n{view}')

P_world = np.array([15, 10, 25, 1])  # Homogeneous coordinates of the point

P_camera = view @ P_world  #= [5 5 5 1], TODO: do not understand how to interpret the result, because the poin lies behind the camera
print(f'P_camera=\n{P_camera}')
