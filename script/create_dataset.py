import os, sys, json

grid_size = 2

def main(args):
	assert len(args) > 1, 'we expect dataset directory path as command line argument'
	dataset_path = args[1]
	
	# Initial dataset configuration file structure
	config = {
		"//": "Describes dataset directory",
		"grid_size": grid_size,
		"elevation": {
			"tile_prefix": "plzen_elev_",
			"pixel_size": 26.063200588611451,
			"tile_size": 716
		},
		"satellite": {
			"tile_prefix": "plzen_rgb_",
			"tile_size": 622
		},
		"//": "file specific data",
		"files": {}
	}

	# TODO: we want pixel_size and tile_size to be autoconfigured

	# create maxvalue list
	for file_name in os.listdir(dataset_path):
		if file_name.startswith(config['elevation']['tile_prefix']) and file_name.endswith('.tif'):
			maxval = tile_max_value(os.path.join(dataset_path, file_name))
			config['files'][file_name] = {'maxval':maxval}

	# save json
	dataset_path = os.path.join(dataset_path, 'dataset.json')
	with open(dataset_path, 'w', encoding='utf-8') as dataset:
		json.dump(config, dataset, indent=2, ensure_ascii=False)

	print(f'{dataset_path} created!')

def tile_max_value(tilepath):
	import subprocess

	command = f"gdalinfo {tilepath} -mm|grep 'Computed Min/Max'| awk -F ',' '{{print $2}}'"
	try:
		result = subprocess.check_output(command, shell=True, text=True)
		return int(float(result))
	except subprocess.CalledProcessError as e:
		print(f"Error running command, what: {e}")
		return

main(sys.argv)
