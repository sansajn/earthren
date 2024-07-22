# prints out OSM tile numbers as (x,y) pairs
import math

def lat_lon_to_tile_numbers(lat, lon, zoom):
	x_tile = math.floor((lon + 180) / 360 * 2**zoom)
	y_tile = math.floor((1 - math.log(math.tan(math.radians(lat)) + 1 / math.cos(math.radians(lat))) / math.pi) / 2 * 2**zoom)
	return x_tile, y_tile

# Plzen coordinates
latitude = 49.7384
longitude = 13.3736
zoom_level = 12

plzen_tile_numbers = lat_lon_to_tile_numbers(latitude, longitude, zoom_level)
print(f'{plzen_tile_numbers}')
