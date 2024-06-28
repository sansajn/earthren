# sample to generate bitmap of normals for z=sin(x)*sin(y) where x,y in [0, pi] surface
import numpy as np
from PIL import Image

image_width = 512
image_height = 512

def normalize(v):
	norm = np.linalg.norm(v)
	return v / norm if norm != 0 else v

def to_word(x, y, z):
	x_w = np.pi/(image_width-1)*x
	y_w = np.pi/(image_height-1)*y
	return np.array([x_w, y_w, z])

def generate_normal_map(heights, normal_map_fname):
	"""Save normal map as \a normal_map_fname png file."""
	rows = heights.shape[0]
	cols = heights.shape[1]
	normals = np.zeros((rows-1, cols-1, 3))  # TODO: it should be -2 there
	for y in range(1, rows-1):
		for x in range(1, cols-1):
			p_xy = to_word(x, y, heights[y,x])
			cos_xy = np.cos(p_xy[0:2])
			sin_xy = np.sin(p_xy[0:2])
			dF = np.array([cos_xy[0]*sin_xy[1], sin_xy[0]*cos_xy[1], -1])
			n = normalize(-dF)
			normals[y,x] = n  # TODO: wrong y-1,x-1

	normals_normalized = (normals + 1)/2  # [-1, 1] to [0, 1]
	image = Image.fromarray((normals_normalized * 255).astype(np.uint8))
	image.save(normal_map_fname)

# Step 1: Generate the surface as 2D array
x = np.linspace(0, np.pi, image_width)
y = np.linspace(0, np.pi, image_height)
X, Y = np.meshgrid(x, y)
Z = np.sin(X) * np.sin(Y)

generate_normal_map(Z, 'sinxy_normals_fce.png')
print('sinxy_normals_fce.png created')

Normalize Z to be in the range [0, 255] for image representation
Z_normalized = (255 * (Z - np.min(Z)) / (np.max(Z) - np.min(Z))).astype(np.uint8)
generate_normal_map(Z_normalized / 255, 'sinxy_normals_fce_normalized.png')
print('sinxy_normals_fce_normalized.png created')

# read heights from bitmap
# now compare saved png data with source data
with Image.open('sinxy.png') as image:
	Z_image = np.array(image)
	generate_normal_map(Z_image / 255, 'out.png')
	print('out.png created')