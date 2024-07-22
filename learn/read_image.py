# example to read png file and work with pixels in numpy
from PIL import Image
import numpy as np

heights_path = '../data/sinxy.png'

# at pixel (230, 465) we expect value of 70
x = 230
y = 465

with Image.open(heights_path) as image:
	heights = np.array(image)
	pixel = heights[y,x]
	print(f'{pixel}')  #= 70

