# OpenGLRayTracer

A basic ray tracer written in C++ using OpenGL, following the [_Ray Tracing in One Weekend_](https://raytracing.github.io/books/RayTracingInOneWeekend.html) book series.

Includes two different modes: A static image renderer (writes to output.ppm), and an interactable scene viewer with first person camera controls.

## Dependencies

- GLFW
- glad
- glm

## Sample Images

![output](https://github.com/TheOneThatFlys/ray-tracer/assets/110343508/521ed41d-b7ef-417f-87b8-bffabe21501c)

Scene with a reflective, glass, and matte ball surrounded by more randomly generated balls.

Took ~27s to render (1920x1080, 256 samples per pixel, 16 ray bounces)
