mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -G"Ninja"
ninja
./sklm_test ../input.jpg output.ppm