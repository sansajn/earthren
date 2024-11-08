# Script can create tiles from specified area (Plzen area hardcoded)
from osgeo import gdal
import numpy as np

# Open the source SRTM tile
src_ds = gdal.Open('/home/ja/gis/data/srtm/n49_e013_1arc_v3.tif')

# Set the target UTM projection (replace EPSG:32633 with your UTM zone)
utm_proj = 'EPSG:32633'  # Example UTM zone, adjust accordingly

# Define your area of interest (replace with your provided coordinates)
projwin_extent_plzen = [372369.741, 5521901.825, 409692.244, 5484579.322]

# TODO: there we want to be sure that extent is fully included into SRC file extent

# Use GDAL's warp function to reproject the tile
dst_ds = gdal.Warp('out/reprojected_tile.tif', src_ds, dstSRS=utm_proj, 
	outputBounds=projwin_extent_plzen)

# Check if cropping was successful
if dst_ds is None:
	raise RuntimeError("Cropping failed. Please check the input bounds and source file.")

# Get raster size and bands
width = dst_ds.RasterXSize
height = dst_ds.RasterYSize
band = dst_ds.GetRasterBand(1)
print(f'raster info: width={width}, height={height}')
# TODO: how many bands we have?

# Define tile size (128x128 pixels)
tile_size = 128
print(f'tile_size={tile_size}')

# Calculate the number of tiles along the x and y axes
tiles_x = width // tile_size  # // floor division operator
tiles_y = height // tile_size
print(f'tiles: {tiles_x}x{tiles_y} (w,h)')

# Loop through each tile, extract and save
for i in range(tiles_x):
	for j in range(tiles_y):
		# Define the window for the tile
		x_offset = i * tile_size
		y_offset = j * tile_size
		window = band.ReadAsArray(x_offset, y_offset, tile_size, tile_size)

		# Create a new file for each tile
		driver = gdal.GetDriverByName('GTiff')
		tile_ds = driver.Create(f'out/tile_{i}_{j}.tif', tile_size, tile_size, 1, gdal.GDT_Float32)

		# Write the window data to the new tile
		tile_ds.GetRasterBand(1).WriteArray(window)

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
