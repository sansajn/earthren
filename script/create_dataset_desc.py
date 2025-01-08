import os, sys, json, subprocess

def main(args):
	assert len(args) > 2, 'we expect dataset directory path as command line argument and grid size'
	dataset_path = args[1]
	grid_size = int(args[2])

	# TODO: elevation, satellite tile_prefix hardcoded in a config, fix that

	# Initial dataset configuration file structure
	config = {
		"// Describes dataset directory": "",
		"grid_size": grid_size,
		"elevation": {
			"tile_prefix": "plzen_elev_",
			"pixel_size": 0.0,
			"tile_size": 0
		},
		"satellite": {
			"tile_prefix": "plzen_rgb_",
			"tile_size": 0
		},
		"// file specific data": "",
		"files": {}
	}

	# TODO: we want pixel_size and tile_size to be autoconfigured

	elevation_info = False
	elevation_pixel_size = 0.0
	elevation_tile_size = 0

	# create maxvalue list and elevation statistics
	for file_name in os.listdir(dataset_path):
		if file_name.startswith(config['elevation']['tile_prefix']) and file_name.endswith('.tif'):
			maxval = tile_max_value(os.path.join(dataset_path, file_name))
			config['files'][file_name] = {'maxval':maxval}
			if not elevation_info:
				elevation_path = os.path.join(dataset_path, file_name)
				elevation_pixel_size = tile_pixel_size(elevation_path)
				elevation_tile_size = tile_size(elevation_path)
				elevation_info = True

	assert elevation_pixel_size > 0, 'we expect elevation pixel-size > 0 && e.g. 26.063'
	assert elevation_tile_size > 0, 'we expect elevation tile-size > 0 e.g. 716'

	config['elevation']['pixel_size'] = elevation_pixel_size
	config['elevation']['tile_size'] = elevation_tile_size

	# create satellite statistics
	satellite_info = False
	satellite_pixel_size = 0.0
	satellite_tile_size = 0

	for file_name in os.listdir(dataset_path):
		if file_name.startswith(config['satellite']['tile_prefix']) and file_name.endswith('.tif'):
			if not satellite_info:
				satellite_path = os.path.join(dataset_path, file_name)
				satellite_pixel_size = tile_pixel_size(satellite_path)
				satellite_tile_size = tile_size(satellite_path)

	assert satellite_pixel_size > 0, 'we expect satellite pixel-size > 0 && e.g. 26.063'
	assert satellite_tile_size > 0, 'we expect satellite tile-size > 0 e.g. 716'

	config['satellite']['pixel_size'] = satellite_pixel_size
	config['satellite']['tile_size'] = satellite_tile_size

	# save json
	dataset_path = os.path.join(dataset_path, 'dataset.json')
	with open(dataset_path, 'w', encoding='utf-8') as dataset:
		json.dump(config, dataset, indent=2, ensure_ascii=False)

	print(f'{dataset_path} created!')

def tile_size(tile_path):
	command = f"gdalinfo {tile_path} -mm|grep 'Size is'|awk -F '[(),]' '{{print $2}}'"
	try:
		result = subprocess.check_output(command, shell=True, text=True)
		return int(result)
	except subprocess.CalledProcessError as e:
		print(f"Error running tile_size command, what: {e}")
		return

def tile_pixel_size(tile_path):
	command = f"gdalinfo {tile_path} -mm|grep 'Pixel Size'|awk -F '[(),]' '{{print $2}}'"
	try:
		result = subprocess.check_output(command, shell=True, text=True)
		return float(result)
	except subprocess.CalledProcessError as e:
		print(f"Error running tile_pixel_size command, what: {e}")
		return

def tile_max_value(tile_path):
	command = f"gdalinfo {tile_path} -mm|grep 'Computed Min/Max'| awk -F ',' '{{print $2}}'"
	try:
		result = subprocess.check_output(command, shell=True, text=True)
		return int(float(result))
	except subprocess.CalledProcessError as e:
		print(f"Error running tile_max_value command, what: {e}")
		return

main(sys.argv)
