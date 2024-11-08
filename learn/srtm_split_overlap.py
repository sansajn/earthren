# Script can create tiles from specified area (Plzen area hardcoded) with overlapping to get rid of artifacts during terrain rendering.
from osgeo import gdal
import math

# TODO: we want input elevation tile is configured via commandline
source_srtm_file = '/home/ja/gis/data/srtm/n49_e013_1arc_v3.tif'
output_directory = 'out'
tile_size = 734 #128
overlap = 1

# Open the source SRTM tile
src_ds = gdal.Open(source_srtm_file)

# Set the target UTM projection (replace EPSG:32633 with your UTM zone)
utm_proj = 'EPSG:32633'  # Example UTM zone, adjust accordingly

# Define your area of interest (replace with your provided coordinates)
projwin_extent_plzen = [372369.741, 5521901.825, 409692.244, 5484579.322]

# TODO: there we want to be sure that extent is fully included into SRC file extent

# Use GDAL's warp function to reproject the tile
dst_ds = gdal.Warp(f'{output_directory}/reprojected_tile.tif', src_ds, dstSRS=utm_proj, 
	outputBounds=projwin_extent_plzen)
# returns osgeo.gdal.Dataset which is python proxy of a GDALDataset see https://gdal.org/en/latest/api/python/raster_api.html#dataset

# Check if cropping was successful
if dst_ds is None:
	raise RuntimeError("Cropping failed. Please check the input bounds and source file.")

# Get raster size and bands
width = dst_ds.RasterXSize
height = dst_ds.RasterYSize
band_count = dst_ds.RasterCount
band = dst_ds.GetRasterBand(1)
print(f'raster info: width={width}, height={height}, bands={band_count}')

# Define tile size (128x128 pixels)
print(f'tile_size={tile_size}')

# Calculate the number of tiles (with overlap) along the x and y axes
tiles_x = math.floor(1 + (width - tile_size)/(tile_size-overlap))
tiles_y = math.floor(1 + (width - tile_size)/(tile_size-overlap))
print(f'tiles: {tiles_x}x{tiles_y} (w,h)')

# Loop through each tile, extract and save
for i in range(tiles_x):
	for j in range(tiles_y):
		# Define the window for the tile
		x_offset = tile_size*i - i
		y_offset = tile_size*j - j
		#y_offset = tile_size + (tile_size-overlap)*(j-1) if j>0 else 0
		#x_offset = i * tile_size
		#y_offset = j * tile_size
		print(f'x_offset={x_offset}, y_offset={y_offset}')
		window = band.ReadAsArray(x_offset, y_offset, tile_size, tile_size)

		# Create a new file for each tile
		driver = gdal.GetDriverByName('GTiff')
		tile_ds = driver.Create(f'{output_directory}/tile_{i}_{j}.tif', tile_size, tile_size, 1, gdal.GDT_Int16)

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
