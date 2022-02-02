# CMakeList.txt : CMake project for TInyRenderer, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

# Add source to this project's executable.
add_executable (TInyRenderer "TInyRenderer.cpp"  "tgaimage.cpp" "tgaimage.h" "model.h" "model.cpp" "geometry.h" "geometry.cpp" "homemade_gl.h" "homemade_gl.cpp")

# TODO: Add tests and install targets if needed.
