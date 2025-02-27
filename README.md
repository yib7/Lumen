To run the files after locating the folder's path...

gcc plane.c light.c ray.c sphere.c vector.c -lm -o rt

then...

./rt > img.ppm

to create the raytraced sphere file.


The current creative.txt creates a line of spheres of various colors, shapes, and sizes that resemble the planets in our solar system.


Feel free to play with it, but do note the format and parameters.

s = Sphere, 
- First line: Center of the sphere (x, y, z)
- Second line: Radius
- Third line: Color (r, g, b)

p = Plane, 
- First line: Normal vector of the plane (x, y, z)
- Second line: Distance
- Third line: Color 1 (r, g, b)
- Fourth line: Color 2 (r, g, b)

l = Light
- First line: Location of the light (x, y, z)
