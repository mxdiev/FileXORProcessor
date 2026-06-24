cmake_minimum_required(VERSION 3.16)
project(FileXORProcessor LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Включаем автоматическую обработку MOC, UIC, RCC
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 COMPONENTS Widgets Core REQUIRED)

add_executable(FileXORProcessor
    main.cpp
    mainwindow.cpp
    mainwindow.h
    fileprocessor.cpp
    fileprocessor.h
)

target_link_libraries(FileXORProcessor PRIVATE Qt::Widgets Qt::Core)