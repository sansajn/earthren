# OSM tile bounding box to shapefile
import math
import geopandas as gpd
from shapely.geometry import Polygon

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

def bbox_to_polygon(bbox):
	return Polygon([
		[bbox[0], bbox[1]],
		[bbox[2], bbox[1]],
		[bbox[2], bbox[3]],
		[bbox[0], bbox[3]],
		[bbox[0], bbox[1]]
	])

plzen_tile = (2200, 1393, 12)
plzen_geometry = bbox_to_polygon(get_tile_bounding_box(*plzen_tile)) 

crs = 'epsg:4326'
polygon = gpd.GeoDataFrame(index=[0], crs=crs, 
	geometry=[plzen_geometry])
polygon.to_file('plzen_tile.shp', driver="ESRI Shapefile")  # this creates shp, shx and prj files
