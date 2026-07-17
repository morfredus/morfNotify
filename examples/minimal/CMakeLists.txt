find_package(Qt6 REQUIRED COMPONENTS Core)

add_executable(morfnotify_demo main.cpp)
target_link_libraries(morfnotify_demo PRIVATE morfNotify::morfNotify Qt6::Core)

if(MSVC)
    target_compile_options(morfnotify_demo PRIVATE /utf-8)
endif()
