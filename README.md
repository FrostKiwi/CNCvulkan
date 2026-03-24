# Full ban on AI
This repo is for me to learn. No character shall touch an LLM.

# volumeVulkan
- scoop install main/mingw-mstorsjo-llvm-ucrt
- Required:
    - cmake
        - Due to https://gitlab.kitware.com/cmake/cmake/-/work_items/27706 it *needs* to be 4.2.3
    - ninja
# Build
- `mkdir` a `build` folder
- `cmake .. -G "Ninja" -D CMAKE_C_COMPILER=clang -D CMAKE_CXX_COMPILER=clang++ -D CMAKE_BUILD_TYPE=Debug`
- Run Ninja
# Notes
Matcaps are public domain and original work files are in the [blender-assets repo](https://projects.blender.org/blender/blender-assets/src/branch/main/work/bundled/matcaps/workfiles)

# Resources:
- [TU Wien Vulkan Playlist](https://www.youtube.com/watch?v=tLwbj9qys18&list=PLmIqTlJ6KsE1Jx5HV4sd2jOe3V1KMHHgun)
