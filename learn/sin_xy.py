# sample to visualize z=sin(x)*sin(y) for x=0..pi, y=0..pi
import numpy as np
import matplotlib.pyplot as plt

# Define the range for x and y
x = np.linspace(0, np.pi, 100)
y = np.linspace(0, np.pi, 100)

# Create a meshgrid
X, Y = np.meshgrid(x, y)

# Compute the sine function
Z = np.sin(X) * np.sin(Y)

p_x = np.pi/2
p_y = np.pi/2
p_z = np.sin(p_x)*np.sin(p_y)

# Plotting the surface plot
fig = plt.figure()
ax = fig.add_subplot(111, projection='3d')
ax.plot_surface(X, Y, Z, cmap='viridis', alpha=0.7)

ax.scatter(p_x, p_y, p_z, color='r', s=100)  # s is a size of the point

# Labels and title
ax.set_xlabel('X')
ax.set_ylabel('Y')
ax.set_zlabel('Z')
ax.set_title('z = sin(x) * sin(y)')

# Show the plot
plt.show()
