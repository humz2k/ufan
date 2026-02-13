find include -iname '*.hpp' -o -iname '*.cpp' | xargs clang-format -i
find drivers -iname '*.hpp' -o -iname '*.cpp' | xargs clang-format -i
find src -iname '*.hpp' -o -iname '*.cpp' | xargs clang-format -i
find . -iname 'CMakeLists.txt' | xargs cmake-format -i