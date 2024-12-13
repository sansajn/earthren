# Script creates four tiles from input tile. Tiles are split from top left corner and saved as tile_{C}_{R}.jpg, where C represents column and R row of a 2x2 grid. This means tile_0_0.tif shows top-left quadrant tile_1_0.tif top-right quadrant, .... Works with GDAL 3.4.3 or 3.8.4.
# usage: split_half_overlap.py [input-source-tif-file]
from osgeo import gdal
import os
from pathlib import Path
import argparse

default_input_tile = '../data/plzen_elev.tif'

def main():
	print(f'GDAL {gdal.__version__}')

	args = parse_commandline()
	num_row_tiles = args.row_tiles
	input_tile = args.input_tile

	print(f'input-tile={input_tile}')

	# Open the source SRTM tile
	src_ds = gdal.Open(input_tile)

	# Set the target UTM projection (replace EPSG:32633 with your UTM zone)
	utm_proj = 'EPSG:32633'  # Example UTM zone, adjust accordingly

	os.makedirs('./out', exist_ok=True)  # ensure output directory

	# Use GDAL's warp function to reproject the tile
	dst_ds = gdal.Warp('out/reprojected_tile.tif', src_ds, dstSRS=utm_proj)

	# Check if cropping was successful
	if dst_ds is None:
		raise RuntimeError("Cropping failed. Please check the input bounds and source file.")

	# Get raster size and bands
	width = dst_ds.RasterXSize
	height = dst_ds.RasterYSize
	band_count = dst_ds.RasterCount
	print(f'raster info: width={width}, height={height}, bands={band_count}')

	# Define tile size
	tile_size = int(width/num_row_tiles)
	print(f'tile_size={tile_size}')

	# Calculate the number of tiles along the x and y axes
	tiles_x = width // tile_size  # // floor division operator
	tiles_y = height // tile_size
	print(f'tiles: {tiles_x}x{tiles_y} (w,h)')

	data_type = dst_ds.GetRasterBand(1).DataType  # we expect all bands have the same data type

	# Loop through each tile, extract and save
	for i in range(tiles_x):
		for j in range(tiles_y):
			# Create a new file for each tile
			tile_name = Path(input_tile).stem
			driver = gdal.GetDriverByName('GTiff')
			tile_ds = driver.Create(f'out/{tile_name}_{i}_{j}.tif', tile_size, tile_size, band_count, data_type)

			x_offset = i * tile_size
			y_offset = j * tile_size

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


def parse_commandline():
	parser = argparse.ArgumentParser(
		description='Split a TIF file into overlapping tiles'
	)

	parser.add_argument(
		'--row-tiles', 
		type=int, 
		required=True, 
		help='Number of tiles in a row to split the input tile into'
	)

	parser.add_argument(  # Add positional argument for input source TIF file
		'input_tile',
		nargs='?',
		default=default_input_tile,
		help=f'Path to the input source TIF file (default: {default_input_tile})'
	)

	args = parser.parse_args()

	# Validate the input file
	if not os.path.exists(args.input_tile):
		parser.error(f"The file {args.input_tile} does not exist")

	if not args.input_tile.endswith('.tif'):
		parser.error("Input file must be a TIF file")

	return args


if __name__ == '__main__':
	main()
