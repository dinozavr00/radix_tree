cmake_minimum_required(VERSION 3.10.0)
project(radix_tree VERSION 0.1.0 LANGUAGES C CXX)

# указываем стандарт C++
set(CMAKE_CXX_STANDARD 20)
# генерация compile_commands.json для clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE INTERNAL "")

# список файлов с исходным кодом, который необходимо скомпилировать
add_library(radix_tree src/radix_tree.cpp)
# директория с заголовочными файлами. С этим можно писать #include <...> вместо #include "..."
target_include_directories(radix_tree PUBLIC include/)


# тестирование
include(CTest)
enable_testing()

# библиотека для тестирования catch2 скачивается автоматически без необходимости ручной установки
include(FetchContent)
FetchContent_Declare(catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.15.2
)
FetchContent_MakeAvailable(catch2)

# список тестов
add_executable(tests tests/test1.cpp)

# зависимости тестов: ваша библиотека и catch2
target_link_libraries(tests PRIVATE Catch2::Catch2WithMain)
target_link_libraries(tests PRIVATE radix_tree)

target_include_directories(tests PRIVATE include/)
