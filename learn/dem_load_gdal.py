# https://dwikita-ichsana.medium.com/meteorology-101-how-to-read-elevation-data-on-basemap-36bb0b6ec7a1
import sys
import matplotlib.pyplot as plt
import numpy as np
from osgeo import gdal

default_dem = '/home/ja/gis/data/srtm/n49_e013_1arc_v3.tif'

def show(data):
    fig = plt.figure(figsize = (12, 8))
    plt.imshow(data, cmap = "terrain", origin='lower') 
                #levels = list(range(0, 1100, 100)))
    plt.colorbar(label='Elevation (meters)')
    plt.gca().set_aspect('equal', adjustable='box')
    plt.title('Digital Elevation Model (DEM)')
    plt.xlabel('X [m]')
    plt.ylabel('Y [m]')
    plt.show()

def main(args):
    filename = args[1] if len(args) > 1 else default_dem

    gdal.UseExceptions()
    gdal_data = gdal.Open(filename)
    gdal_band = gdal_data.GetRasterBand(1)
    nodataval = gdal_band.GetNoDataValue()

    info = gdal.Info(filename)
    print(info)

    # convert to a numpy array
    data_array = gdal_data.ReadAsArray().astype(float)

    # replace missing values if necessary
    if np.any(data_array == nodataval):
        data_array[data_array == nodataval] = np.nan

    data_array_flip = np.flipud(data_array)
    show(data_array_flip)

main(sys.argv)
