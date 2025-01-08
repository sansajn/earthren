"""
Renames output tile from split_tile_overlap.py utility for a next tree (LOD) level e.g. `tile_1_0_0_1.tif` -> `tile_2_1.tif`.
"""
import sys, os, re

if __name__ == '__main__':
	if len(sys.argv) != 2:
		print('Usage: python rename_tile.py <file_path>')
		sys.exit(1)

	tile_path = sys.argv[1]

	if not os.path.isfile(tile_path):
		print(f'The path {tile_path} is not a valid file.')
		sys.exit(1)
	
	directory, filename = os.path.split(tile_path)
	pattern = re.compile(r'^(.+?)_(\d)_(\d)_(\d)_(\d)\.tif$')
   
	match = pattern.match(filename)
	if match:
		prefix = match.group(1)
		A = int(match.group(2))
		B = int(match.group(3))
		C = int(match.group(4))
		D = int(match.group(5))
		
		# Calculate new column and row values
		E = 2 * A + C
		F = 2 * B + D

		new_tilename = f'{prefix}_{E}_{F}.tif'

		# New file path
		new_tile_path = os.path.join(directory, new_tilename)

		# Rename the file
		os.rename(tile_path, new_tile_path)
		#print(f'Renamed {tile_path} to {new_tile_path}')
	else:
		print(f'error: {filename} does not match the expected pattern.')
