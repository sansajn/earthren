import matplotlib.pyplot as plt
import numpy as np

# Function to draw a 3D vector
def draw_vector(ax, origin, direction, color, label):
    ax.quiver(
        origin[0], origin[1], origin[2],
        direction[0], direction[1], direction[2],
        color=color, label=label
    )

# Set up the figure and 3D axis
fig = plt.figure()
ax = fig.add_subplot(111, projection='3d')

# Define the initial vectors and parameters
po = np.array([0, 0, 0])
px = np.array([1, 0, 0])
py = np.array([0, 1, 0])
pz = np.array([0, 0, 1])

# Angles in radians
phi = np.radians(30)  # azimuthal angle
theta = np.radians(45)  # polar angle

# Cosines and sines of angles
ct = np.cos(theta)
st = np.sin(theta)
cp = np.cos(phi)
sp = np.sin(phi)

# Calculate transformed axes
cx = px * cp - pz * sp
cy = py * ct + pz * st * cp + px * st * sp
cz = -py * st + pz * ct * cp + px * ct * sp

# Calculate camera position
d = 10  # distance
zoom = 1  # zoom factor
position = po + cz * d * zoom

# Draw the initial vectors
draw_vector(ax, po, px, 'r', 'px')
draw_vector(ax, po, py, 'r', 'py')
draw_vector(ax, po, pz, 'r', 'pz')

# Draw the transformed axes
draw_vector(ax, po, cx, 'g', 'cx (transformed)')
draw_vector(ax, po, cy, 'g', 'cy (transformed)')
draw_vector(ax, po, cz, 'g', 'cz (transformed)')

# Draw the camera position
ax.scatter(position[0], position[1], position[2], color='k', label='Camera Position')

# Setting the labels
ax.set_xlabel('X')
ax.set_ylabel('Y')
ax.set_zlabel('Z')
ax.legend()

# Set the aspect ratio
ax.set_box_aspect([1,1,1])

# Display the plot
plt.show()
