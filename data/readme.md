`dem.tif`: downloaded by QGis via *OpenTopography DEM Downloader* plugin for an area defined by a shapefile generated with `learn/bbox_shapefile.py` script. The bounding box covers tiles from `tiles` directory calculated by `learn/bbox_calc.py` script. Source for height map is SRTM 30m service.

Some more information about `dem.tiff`

```console
$ gdalinfo -mm dem.tiff
Driver: GTiff/GeoTIFF
Files: dem.tiff
Size is 1582, 1016
Origin = (14.238194443999999,50.177083332999999)
Pixel Size = (0.000277777778129,-0.000277777777559)
Image Structure Metadata:
  INTERLEAVE=BAND
Corner Coordinates:
Upper Left  (  14.2381944,  50.1770833)
Lower Left  (  14.2381944,  49.8948611)
Upper Right (  14.6776389,  50.1770833)
Lower Right (  14.6776389,  49.8948611)
Center      (  14.4579167,  50.0359722)
Band 1 Block=1582x2 Type=Int16, ColorInterp=Gray
    Computed Min/Max=148.000,535.000
  NoData Value=-32768
```


`tiles`: manually downloaded area covers prague