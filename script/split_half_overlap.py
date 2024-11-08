# Script creates four tiles from input tile with 1px overlapping to get rid of artifacts during terrain rendering. Workins with GDAL 3.4.3. Tiles are split from top left corner and saved as tile_{C}_{R}.jpg, where C represents column and R row of a 2x2 grid. This means tile_0_0.tif shows top-left quadrant tile_1_0.tif top-right quadrant, .... Works with GDAL 3.4.3 or 3.8.4.
# usage: split_half_overlap.py [input-source-tif-file]
from osgeo import gdal
import math
import sys
import os
from pathlib import Path

print(f'GDAL {gdal.__version__}')

default_input_tile = '../data/plzen_rgb.tif'

input_tile = sys.argv[1] if len(sys.argv) > 1 else default_input_tile

print(f'input-tile={input_tile}')

# TODO: we want input elevation tile is configured via commandline
output_directory = 'out'
overlap = 1

# Open the source tile
src_ds = gdal.Open(input_tile)

# Set the target UTM projection (replace EPSG:32633 with your UTM zone)
utm_proj = 'EPSG:32633'  # Example UTM zone, adjust accordingly

os.makedirs('./out', exist_ok=True)  # ensure output directory

# Use GDAL's warp function to reproject the tile
dst_ds = gdal.Warp(f'{output_directory}/reprojected_tile.tif', src_ds, dstSRS=utm_proj)
# returns osgeo.gdal.Dataset which is python proxy of a GDALDataset see https://gdal.org/en/latest/api/python/raster_api.html#dataset

# Check if cropping was successful
if dst_ds is None:
	raise RuntimeError("Cropping failed. Please check the input bounds and source file.")

# Get raster size and bands
width = dst_ds.RasterXSize
height = dst_ds.RasterYSize
band_count = dst_ds.RasterCount
print(f'raster info: width={width}, height={height}, bands={band_count}')

tile_size = int(width/2)
print(f'tile_size={tile_size}')

# Calculate the number of tiles (with overlap) along the x and y axes
tiles_x = math.floor(1 + (width - tile_size)/(tile_size-overlap))
tiles_y = math.floor(1 + (width - tile_size)/(tile_size-overlap))
print(f'tiles: {tiles_x}x{tiles_y} (w,h)')

data_type = dst_ds.GetRasterBand(1).DataType  # we expect all bands have the same data type

# Loop through each tile, extract and save
for i in range(tiles_x):
	for j in range(tiles_y):
		# Create a new file for each tile
		tile_name = Path(input_tile).stem
		driver = gdal.GetDriverByName('GTiff')
		tile_ds = driver.Create(f'out/{tile_name}_{i}_{j}.tif', tile_size, tile_size, band_count, data_type)

		# Define the window for the tile
		x_offset = tile_size*i - i
		y_offset = tile_size*j - j

		for band_idx in range(1,band_count+1):  # for each band
			band = dst_ds.GetRasterBand(band_idx)

			# Define the window for the tile
			window = band.ReadAsArray(x_offset, y_offset, tile_size, tile_size)

			# Write the window data to the new tile
			tile_ds.GetRasterBand(band_idx).WriteArray(window)

		# Set the georeferencing information (adjust it based on the UTM projection)
		geo_transform = dst_ds.GetGeoTransform()
		tile_transform = (
			geo_transform[0] + x_offset * geo_transform[1],
			geo_transform[1],
			0.0,
			geo_transform[3] + y_offset * geo_transform[5],
			0.0,
         geo_transform[5]
		)
		tile_ds.SetGeoTransform(tile_transform)
		tile_ds.SetProjection(dst_ds.GetProjection())

		# Close the dataset to flush to disk
		tile_ds = None

# Close the source dataset
dst_ds = None
