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
	with rasterio.open(dem_path) as dataset:  # -> DatasetReader
		# print some basic info
		print(dataset)
		print(f'bounds={dataset.bounds}')
		print(f'bands={dataset.count}')
		print(f'CRS={dataset.crs}')
		
		dem_data = dataset.read(1)  # -> Numpy ndarray or a view on a Numpy ndarray

	# Plot the DEM data
	plt.figure(figsize=(10, 10))
	plt.imshow(dem_data, cmap='terrain')
	plt.colorbar(label='Elevation (meters)')
	plt.title('Digital Elevation Model (DEM)')
	plt.xlabel('Longitude')
	plt.ylabel('Latitude')
	plt.show()

main(sys.argv)
