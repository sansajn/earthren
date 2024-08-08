# Loading DEM data sample, this implementation can't correct visualize 
# projected files (probably due to nodata), see `dem_load_gdal.py`.
import sys
import rasterio
import numpy as np
import matplotlib.pyplot as plt

default_dem = '../data/dem.tiff'

def main(args):
	dem_path = args[1] if len(args) > 1 else default_dem

	# Load the DEM data
	with rasterio.open(dem_path) as src:
		dem_data = src.read(1)

	# Plot the DEM data
	plt.figure(figsize=(10, 10))
	plt.imshow(dem_data, cmap='terrain')
	plt.colorbar(label='Elevation (meters)')
	plt.title('Digital Elevation Model (DEM)')
	plt.xlabel('Longitude')
	plt.ylabel('Latitude')
	plt.show()

main(sys.argv)
