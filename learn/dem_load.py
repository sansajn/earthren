# loading DEM data sample
import rasterio
import matplotlib.pyplot as plt

# Load the DEM data
dem_path = '../data/dem.tiff'
with rasterio.open(dem_path) as src:
	dem_data = src.read(1)
	dem_transform = src.transform

# Plot the DEM data
plt.figure(figsize=(10, 10))
plt.imshow(dem_data, cmap='terrain')
plt.colorbar(label='Elevation (meters)')
plt.title('Digital Elevation Model (DEM)')
plt.xlabel('Longitude')
plt.ylabel('Latitude')
plt.show()
