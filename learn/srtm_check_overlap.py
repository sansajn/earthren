from osgeo import gdal
import numpy as np

# Load the two tiles
tile1_path = 'out/tile_0_0.tif'
tile2_path = 'out/tile_1_0.tif'

# Open the first and second tile datasets
tile1 = gdal.Open(tile1_path)
tile2 = gdal.Open(tile2_path)

print(f'width={tile1.RasterXSize}, height={tile1.RasterYSize}')

# Read the last column of the first tile
tile1_band = tile1.GetRasterBand(1)  # Assuming single-band raster
tile1_last_col = tile1_band.ReadAsArray(xoff=tile1.RasterXSize - 1, yoff=0, win_xsize=1, win_ysize=tile1.RasterYSize)

# Read the first column of the second tile
tile2_band = tile2.GetRasterBand(1)
tile2_first_col = tile2_band.ReadAsArray(xoff=0, yoff=0, win_xsize=1, win_ysize=tile2.RasterYSize)

# Check if they are equal
are_equal = np.array_equal(tile1_last_col, tile2_first_col)

# Calculate the differences
differences = tile1_last_col - tile2_first_col

# Count the number of differing pixels
different_pixels_count = np.count_nonzero(differences)

print(f"Number of differing pixels: {different_pixels_count}")

# Output the result
if are_equal:
    print("The last column of the first tile is equal to the first column of the second tile.")
else:
    print("The last column of the first tile is not equal to the first column of the second tile.")

# Close the datasets
tile1 = None
tile2 = None
