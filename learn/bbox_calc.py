# taken from chatbot and getting correct results
import math

def tile_to_lon_lat(x, y, zoom):
    # Convert tile coordinates to longitude and latitude
    n = 2.0 ** zoom
    lon_deg = x / n * 360.0 - 180.0
    lat_rad = math.atan(math.sinh(math.pi * (1 - 2 * y / n)))
    lat_deg = math.degrees(lat_rad)
    return lon_deg, lat_deg

def get_tile_bounding_box(x, y, zoom):
    # Top-left corner
    lon_tl, lat_tl = tile_to_lon_lat(x, y, zoom)
    # Bottom-right corner
    lon_br, lat_br = tile_to_lon_lat(x + 1, y + 1, zoom)
    return (lon_tl, lat_tl, lon_br, lat_br)

plzen_tile = (2200, 1393, 12)
prague_tile = (2210, 1386, 12)

prague_bbox = get_tile_bounding_box(*prague_tile)
print(f'Prague bounding box: {prague_bbox}')

plzen_bbox = get_tile_bounding_box(*plzen_tile)
print(f'Plzen bounding box: {plzen_bbox}')

zoom = 12
from_x = 2210
to_x = 2214
from_y = 1386
to_y = 1390

min_lat = 360
max_lat = 0
min_lon = 360
max_lon = 0
for y in range(from_y, to_y+1):
    for x in range(from_x, to_x+1):
        bbox = get_tile_bounding_box(x, y, zoom)
        min_lon = min(min(min_lon, bbox[0]), bbox[2])
        min_lat = min(min(min_lat, bbox[1]), bbox[3])
        max_lon = max(max(max_lon, bbox[0]), bbox[2])
        max_lat = max(max(max_lat, bbox[1]), bbox[3])

print('area bounding-box: ', (min_lat, min_lon, max_lat, max_lon))
