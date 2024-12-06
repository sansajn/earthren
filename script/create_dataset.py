import os, json

def main():
	# Initial dataset configuration file structure
	config = {
		"//": "Describes dataset directory",
		"grid_size": 2,
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

	# TODO: make this cmd argument option
	dataset_path = '../data/gen/grid_of_terrains'
	
	# create maxvalue list
	for file_path in os.listdir(dataset_path):
		if file_path.startswith(config['elevation']['tile_prefix']) and file_path.endswith('.tif'):
			maxval = tile_max_value(os.path.join(dataset_path, file_path))
			config['files'][file_path] = {'maxval':maxval}

	# save json
	dataset_path = os.path.join(dataset_path, 'dataset.json')
	with open(dataset_path, 'w') as dataset:
		json.dump(config, dataset, indent=2)

	print(f'{dataset_path} created!')

def tile_max_value(tilepath):
	# TODO: we need proper implementation
	import random
	return random.randint(100, 500)

main()
