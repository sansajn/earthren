# under construction...
# example to read sinxy.png height map and calculate normals
from PIL import Image
import numpy as np
import matplotlib.pyplot as plt

heights_path = 'sinxy256.png'

def normalize(v):
	norm = np.linalg.norm(v)
	return v / norm if norm != 0 else v

def to_word(x, y, z):
	x_w = np.pi/(heights_w-1)*x
	y_w = np.pi/(heights_h-1)*y
	return np.array([x_w, y_w, z])

def compute_normal(uv, heights):
	uv_left = uv + [-1, 0]
	uv_right = uv + [1, 0]
	uv_up = uv + [0, -1]
	uv_down = uv + [0, 1]
	p0 = to_word(*uv_left, heights[uv_left[1], uv_left[0]])
	p1 = to_word(*uv_right, heights[uv_right[1], uv_right[0]])
	p2 = to_word(*uv_up, heights[uv_up[1], uv_up[0]])
	p3 = to_word(*uv_down, heights[uv_down[1], uv_down[0]])
	dx = p1 - p0
	dy = p3 - p2
	n = normalize(np.cross(dx, dy))
	return n

def to_word_xy(x, y):
	x_w = np.pi/(heights_w-1)*x
	y_w = np.pi/(heights_h-1)*y
	return np.array([x_w, y_w])

def get_heights_fce(rows, cols):
	x = np.linspace(0, np.pi, cols)
	y = np.linspace(0, np.pi, rows)
	X, Y = np.meshgrid(x, y)
	Z = np.sin(X) * np.sin(Y)
	return Z

def compute_normals_fce(rows, cols):
	"""Compute normal map from height map with a exact function.
	Return normals."""
	normals = np.zeros((rows-2, cols-2, 3))
	for y in range(1, rows-1):
		for x in range(1, cols-1):
			p_xy = to_word_xy(x, y)
			cos_xy = np.cos(p_xy)
			sin_xy = np.sin(p_xy)
			dF = np.array([cos_xy[0]*sin_xy[1], sin_xy[0]*cos_xy[1], -1])
			n = normalize(-dF)
			normals[y-1,x-1] = n
	return normals

def compute_normals_diff(heights):
	"""Compute normal map from height map with neightbord values.
	Return normals."""
	rows = heights.shape[0]
	cols = heights.shape[1]
	normals = np.zeros((rows-2, cols-2, 3))
	for y in range(1, rows-1):
		for x in range(1, cols-1):
			uv = np.array([x,y])
			n = compute_normal(uv, heights)
			normals[y-1,x-1] = n
	return normals

def save_as(normals, normal_map_fname):
	normals_normalized = (normals + 1)/2  # normalize from [-1,1] to [0,1] range
	img = Image.fromarray((normals_normalized * 255).astype(np.uint8))
	img.save(normal_map_fname)
	
def show_diff(a, b):
	a_img = ((a+1)/2)*255
	b_img = ((b+1)/2)*255
	diff = np.abs(a_img - b_img)

	# Plot the original arrays and the difference
	fig, axes = plt.subplots(1, 3, figsize=(a.shape[0], a.shape[1]))

	axes[0].imshow(a_img.astype(np.uint8))
	axes[0].set_title("Normals (fce)")

	axes[1].imshow(b_img.astype(np.uint8))
	axes[1].set_title("Normals (diff)")

	diff_normalized = (255 * (diff - np.min(diff)) / (np.max(diff) - np.min(diff))).astype(np.uint8)
	axes[2].imshow(diff_normalized)
	axes[2].set_title("Normalized Difference")

	plt.show()

# TODO: this is super slow, why?
with Image.open(heights_path) as image:
	heights = np.array(image) / 255  # x,y,z
	heights_h, heights_w = heights.shape

	normals_fce = compute_normals_fce(heights_h, heights_w)
	normals_diff = compute_normals_diff(heights)

	heights_fce = get_heights_fce(heights_h, heights_w)
	normals_diff_from_fce_heights = compute_normals_diff(heights_fce)
		
	allclose = np.allclose(normals_fce, normals_diff)
	print(f'allcloce={allclose}')  #= False

	# save normals
	save_as(normals_diff, 'sinxy_normals.png')
	print(f'normals saved to sinxy_normals.png file')

	save_as(normals_fce, 'sinxy_normals_fce.png')
	print(f'normals saved to sinxy_normals_fce.png file')

	save_as(normals_diff_from_fce_heights, 'sinxy_normals_heights_fce.png')
	print(f'normals saved to sinxy_normals_heights_fce.png file')

	show_diff(normals_fce, normals_diff)
	
