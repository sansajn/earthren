# About
for more details see earth rendering notes ...

# Samples

## `xy_plane_grid_textured`

The latest sample is `xy_plane_grid_textured.cpp` which shows grid of xy_planes the same as previous `xy_plane_grid.cpp` sample but with a texture applied.

The problem there is that pane is not smooth it has a kind of momentum. Affected samples are `xy_plane_grid.cpp`, `xy_plane_grid` and `xy_plane_panzoom` samples.

Another issue is that it is hard to diagnose issues in a shader program, see issue 1.

ToDo:
- rewrite pan functionality
- use VAOs (see `texture_torage` as an example)


Then next step is to implement a camera which would allow smooth pane and zoom. I would like to implement *Proland* like camera.

Then next step is to add heights, I need to figure out area for sample tiles (see `data/tiles`), download heights for that area and spit it to tiles. Then modify shader to add texture (ortho) based heights (in this step without any noise or upsampling).


## `xy_plane_grid`

Shows grid of xy_planes with a zoom and pane features. There is not any camera implementation there.

The problem there is that pane is not smooth it has a kind of momentum. Affected samples are `xy_plane_grid` and `xy_plane_panzoom` samples.

ToDo:
- rewrite pan functionality

Next step is to implement a grid of xy_planes rendered with a image inside (instead fixed color). To see how to render a image see `texture_storage` sample and how to render to a xy plane see `xy_plane_texture` sample.

Then Next step is to implement a camera which would allow smooth pane and zoom.


## `xy_plane_texture`

Shows how to render xy plane fillet with an image

Problems: 
	- we should use VAOs

## `texture_storage`

Shows how to render a picture to screen.


## `xy_plane_panzoom`

xy plane with zoom and pane capabilities


## `xy_plane`

Just a simple red xy plane rendered.


# Issues

1. diagnostic shader issues is hard e.g. during `xy_plane_texture` development I've got followring fragment shader issue

```console
./xy_plane_texture 
GL_VENDOR: Mesa
GL_VERSION: OpenGL ES 3.2 Mesa 23.2.1-1ubuntu3.1~22.04.2
GL_RENDERER: llvmpipe (LLVM 15.0.7, 128 bits)
GLSL_VERSION: OpenGL ES GLSL ES 3.20
ERROR::SHADER::FRAGMENT::COMPILATION_FAILED
0:8(15): error: too many parameters to `vec4' constructor

ERROR::SHADER::PROGRAM::LINKING_FAILED
error: linking with uncompiled/unspecialized shader
```

where fragment shader is defined in a source code following way

```cpp
GLchar const * fragment_shader_source = R"(
#version 320 es
precision mediump float;
in vec2 tex_coord;
out vec4 frag_color;
uniform sampler2D s;
void main() {
    frag_color = vec4(texture(s, tex_coord), 1.0);
})";
```

and from the linker output it is not clear wat goes wrong (what is line 8?).