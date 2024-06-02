# sample to save plot to png file

import numpy as np
import matplotlib.pyplot as plt

# Step 1: Create a 2D NumPy array
data = np.random.rand(10, 10)  # Example: a 10x10 array with random values

# Step 2: Plot the array using matplotlib and save it as a PNG image
plt.imshow(data, cmap='viridis')  # You can choose other colormaps as well
plt.colorbar()  # Adds a color bar to show the scale
plt.title('2D NumPy Array Visualization')

# Save the figure
plt.savefig('2d_array.png')

# Display the plot
plt.show()
