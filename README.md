# boxworld_cpp

A C++ implementation of the Box-World environment from [Relational Deep Reinforcement Learning](https://arxiv.org/pdf/1806.01830.pdf).

## Include to Your Project: CMake

### FetchContent
```shell
include(FetchContent)
# ...
FetchContent_Declare(boxworld
    GIT_REPOSITORY https://github.com/tuero/boxworld_cpp.git
    GIT_TAG master
)

# make available
FetchContent_MakeAvailable(boxworld)
link_libraries(boxworld)
```

### Git Submodules
```shell
# assumes project is cloned into external/boxworld_cpp
add_subdirectory(external/boxworld_cpp)
link_libraries(boxworld)
```

## Installing Python Bindings
```shell
git clone https://github.com/tuero/boxworld_cpp.git
pip install ./boxworld_cpp
```

If you get a `GLIBCXX_3.4.XX not found` error at runtime, 
then you most likely have an older `libstdc++` in your virtual environment `lib/` 
which is taking presidence over your system version.
Either correct your `$PATH`, or update your virtual environment `libstdc++`.

For example, if using anaconda
```shell
conda install conda-forge::libstdcxx-ng
```

## Generate Levels
The build script is taken from [here](https://github.com/nathangrinsztajn/Box-World/).
```shell
cd scripts
python generate_levelset.py --export_path=EXPORT_PATH --map_size=16 --num_train=50000 --num_test=100 --goal_length=5 --num_distractor=2 --distractor_length=3
```
