import numpy as np
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d.art3d import Poly3DCollection

# Define the three points of the triangle
points = [
    [0, 0, 0],  # Point A
    [1, 0, 0],  # Point B
    [0, 1, 0],  # Point C
]

# Extract X, Y, Z coordinates
x = [p[0] for p in points]
y = [p[1] for p in points]
z = [p[2] for p in points]

# Compute the normal vector
vec1 = np.array(points[1]) - np.array(points[0])
vec2 = np.array(points[2]) - np.array(points[0])
normal = np.cross(vec1, vec2)
normal = normal / np.linalg.norm(normal)  # Normalize the vector

# Create a 3D plot
fig = plt.figure()
ax = fig.add_subplot(111, projection='3d')

# Plot the points
ax.scatter(x, y, z, color='r')

# Plot the edges of the triangle
verts = [list(zip(x, y, z))]
triangle = Poly3DCollection(verts, alpha=0.25, facecolor='cyan')
ax.add_collection3d(triangle)

# Plot the normal vector
# Use the centroid of the triangle as the starting point of the normal vector
centroid = np.mean(points, axis=0)
ax.quiver(centroid[0], centroid[1], centroid[2], 
          normal[0], normal[1], normal[2], 
          length=1.0, color='g', linewidth=2)

# Set labels for axes
ax.set_xlabel('X')
ax.set_ylabel('Y')
ax.set_zlabel('Z')

# Set the limits of the axes
ax.set_xlim([min(x)-1, max(x)+1])
ax.set_ylim([min(y)-1, max(y)+1])
ax.set_zlim([min(z)-1, max(z)+1])

# Show plot
plt.show()
