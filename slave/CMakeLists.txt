cmake_minimum_required(VERSION 3.16)

project(TelgrafApp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Qt paketlerini bul
find_package(Qt6 REQUIRED COMPONENTS Widgets SerialPort)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

# Kaynak kodları ekle
add_executable(TelgrafApp main.cpp)

# Kütüphaneleri bağla
target_link_libraries(TelgrafApp PRIVATE Qt6::Widgets Qt6::SerialPort)