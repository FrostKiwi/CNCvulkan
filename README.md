# volumeVulkan
- scoop install main/mingw-mstorsjo-llvm-ucrt
- Required: `cmake` `ninja`, but cmake needs to be in Windows as to get correct Backends
# Build
- `mkdir` a `build` folder
- `cmake .. -G "Ninja" -D CMAKE_C_COMPILER=clang -D CMAKE_CXX_COMPILER=clang++ -D CMAKE_BUILD_TYPE=Debug`
- Run Ninja
# Full ban on AI
This repo is for me to learn. No character shall touch an LLM.
# TODO
- Rewrite with VULKAN_HPP_NO_STRUCT_CONSTRUCTORS 
  - Check against https://docs.vulkan.org/tutorial/latest/03_Drawing_a_triangle/00_Setup/00_Base_code.html and https://docs.vulkan.org/tutorial/latest/02_Development_environment.html#cmake
