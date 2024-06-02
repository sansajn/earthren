# program creates shapefile out of the geo-rectangle
import geopandas as gpd
from shapely.geometry import Polygon


# Or make sure to get a rectangle by using the bounding box of the initial polygon
rectangle_polygon_geometry = Polygon([
	[14.23828125, 49.89463439573421],
	[14.677734375, 49.89463439573421],
	[14.677734375, 50.17689812200106],
	[14.23828125, 50.17689812200106],
	[14.23828125, 49.89463439573421]
])

crs = 'epsg:4326'
polygon = gpd.GeoDataFrame(index=[0], crs=crs, geometry=[rectangle_polygon_geometry])
polygon.to_file('dem.shp', driver="ESRI Shapefile")
