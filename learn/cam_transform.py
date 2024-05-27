import numpy as np
import matplotlib.pyplot as plt

px = np.array([1, 0, 0])
py = np.array([0, 1, 0])
pz = np.array([0, 0, 1])

phi = np.radians(30)  # rotate around y-axis
theta = np.radians(0)

ct = np.cos(theta)
st = np.sin(theta)
cp = np.cos(phi)
sp = np.sin(phi)

# transformed axis
cx = px*cp + py*sp
cy = -px*sp*ct + py*cp*ct + pz*st
cz = px*sp*st - py*cp*st + pz*ct

print(f'cx={cx}')
print(f'cy={cy}')
print(f'cz={cz}')

fig = plt.figure()
ax = fig.add_subplot(111, projection='3d')

ax.quiver(*[0, 0, 0], *px, color='y')
ax.quiver(*[0, 0, 0], *py, color='y')
ax.quiver(*[0, 0, 0], *pz, color='y')

ax.quiver(*[0, 0, 0], *cx, color='r')
ax.quiver(*[0, 0, 0], *cy, color='g')
ax.quiver(*[0, 0, 0], *cz, color='b')

# Setting the labels
ax.set_xlabel('X')
ax.set_ylabel('Y')
ax.set_zlabel('Z')
ax.legend()

# Set the aspect ratio
ax.set_box_aspect([1,1,1])

# Display the plot
plt.show()
