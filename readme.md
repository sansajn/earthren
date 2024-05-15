for more details see earth rendering notes ...

The latest sample is `xy_plane_grid.cpp` which shows grid of xy_planes with a zoom and pane features. There is not any camera implementation there.

The problem there is that pane not smooth it has a kind of momentum. Affected samples are `xy_plane_grid` and `xy_plane_panzoom` samples.

ToDo:
- rewrite pan functionality

Next step is to implement a grid of xy_planes rendered with a image inside (instead fixed color). To see how to render a image see `texture_storage` sample.

Then Next step is to implement a camera which would allow smooth pane and zoom.

`texture_storage`: shows how to render a picture to screen

`xy_plane_panzoom`: xy plane with zoom and pane capabilities

`xy_plane`: just a simple red xy plane rendered
