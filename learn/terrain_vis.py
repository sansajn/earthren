# terrain visualisation in libmathplot
import rasterio
import numpy as np
import matplotlib.pyplot as plt

dem_path = '../data/dem.tiff'

# Load the DEM data
with rasterio.open(dem_path) as src:
    dem_data = src.read(1)
    dem_transform = src.transform

# Create a grid of coordinates for the DEM data
x = np.linspace(0, dem_data.shape[1], dem_data.shape[1])
y = np.linspace(0, dem_data.shape[0], dem_data.shape[0])
x, y = np.meshgrid(x, y)

# Create a 3D plot
fig = plt.figure(figsize=(10, 10))
ax = fig.add_subplot(111, projection='3d')

# Plot the DEM data
ax.plot_surface(x, y, dem_data, cmap='terrain')

ax.set_xlabel('Longitude (X)')
ax.set_ylabel('Latitude (Y)')
ax.set_zlabel('Heights (Z)')

plt.show()
