cmake_minimum_required(VERSION 3.20)

project(TelgrafApp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 COMPONENTS Widgets SerialPort Bluetooth REQUIRED)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

add_executable(TelgrafApp
    main.cpp
)

target_link_libraries(TelgrafApp PRIVATE
    Qt6::Widgets
    Qt6::SerialPort
    Qt6::Bluetooth
)

if(WIN32)
    set_target_properties(TelgrafApp PROPERTIES WIN32_EXECUTABLE ON)
endif()

if(APPLE)
    set_target_properties(TelgrafApp PROPERTIES MACOSX_BUNDLE ON)
endif()

install(TARGETS TelgrafApp DESTINATION bin)

add_custom_command(TARGET TelgrafApp POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:TelgrafApp>/.config
    
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${CMAKE_CURRENT_SOURCE_DIR}/.config/Command.json
    $<TARGET_FILE_DIR:TelgrafApp>/.config/Command.json
    
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${CMAKE_CURRENT_SOURCE_DIR}/.config/style.qss
    $<TARGET_FILE_DIR:TelgrafApp>/.config/style.qss

        COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${CMAKE_CURRENT_SOURCE_DIR}/.config/keybinding.conf
    $<TARGET_FILE_DIR:TelgrafApp>/.config/keybinding.conf

    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${CMAKE_CURRENT_SOURCE_DIR}/.config/TelgrafApp.conf
    $<TARGET_FILE_DIR:TelgrafApp>/.config/TelgrafApp.conf
    
    COMMENT "Config, Style and Settings files copied"
)