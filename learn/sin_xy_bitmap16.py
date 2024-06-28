# sample to generate 16 bit bitmap of z=sin(x)*sin(y) for x,y in [0, pi] values
import numpy as np
from PIL import Image

image_size = 512
image_path = f'sinxy{image_size}_16.png'

uint16_max = np.iinfo(np.uint16).max

# Step 1: Generate the 2D array
x = np.linspace(0, np.pi, image_size)
y = np.linspace(0, np.pi, image_size)
X, Y = np.meshgrid(x, y)
Z = np.sin(X) * np.sin(Y)

# Normalize Z to be in the range [0, uint16_max] for image representation
Z_normalized = (uint16_max * (Z - np.min(Z)) / (np.max(Z) - np.min(Z))).astype(np.uint16)

# Step 2: Save the 2D array as a PNG image
image = Image.fromarray(Z_normalized)
image.save(image_path)
print(f'bitmap saved as "{image_path}"')
