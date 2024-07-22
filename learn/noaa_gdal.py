# reading Plzen area elevations from NOAA data with GDAL and numpy
import math
from osgeo import gdal
import numpy as np

plzen_bbox = (13.359375, 49.78126405817835, 13.447265625, 49.72447918871299)  # (lon, lat)

dataset = gdal.Open('/home/ja/gis/data/noaa/g10g')

width = dataset.RasterXSize
height = dataset.RasterYSize
print(f'size=({width}, {height})')

# affine transformation to map (x,y) raster coordinate into (lon, lat) coordinates
transform = dataset.GetGeoTransform()
print(f'geo-transform={transform}')

# get inverse transformation to map (lon, lat) into (x,y) 
transform_inverse = gdal.InvGeoTransform(transform)
print(f'geo-transform_inverse={transform_inverse}')

# calculate tile extent
lon_min, lat_max = gdal.ApplyGeoTransform(transform, 0, 0)
lon_max, lat_min = gdal.ApplyGeoTransform(transform, width, height)
print(f'a=({lon_min}, {lat_min})')
print(f'b=({lon_max}, {lat_max})')

# note: Prague is outside of the tile area, that is why Plzen
lon_pl0, lat_pl0 = plzen_bbox[0], plzen_bbox[1]
lon_pl1, lat_pl1 = plzen_bbox[2], plzen_bbox[3]
x_pl0, y_pl0 = gdal.ApplyGeoTransform(transform_inverse, lon_pl0, lat_pl0)
x_pl1, y_pl1 = gdal.ApplyGeoTransform(transform_inverse, lon_pl1, lat_pl1)
print(f'({x_pl0}, {y_pl0}), ({x_pl1}, {y_pl1})')

# read part of the elevation data
width_pl = math.ceil(x_pl1) - int(x_pl0) + 1
height_pl = math.ceil(y_pl1) - int(y_pl0) + 1
print(f'plzen tile={width_pl}x{height_pl}')

band = dataset.GetRasterBand(1)

x0 = int(x_pl0)
y0 = int(y_pl0)  # x0,y0 top-left
plzen_elev = band.ReadAsArray(x0, y0, width_pl, height_pl)  #= np.ndarray
print(f'{plzen_elev}')
