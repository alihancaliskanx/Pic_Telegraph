cmake_minimum_required(VERSION 3.16)

project(TelgrafApp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Qt6 paketlerini bul (Bluetooth eklendi)
find_package(Qt6 COMPONENTS Widgets SerialPort Bluetooth REQUIRED)

# Otomatik MOC, UIC ve RCC işlemlerini aç
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Kaynak dosyalar
set(PROJECT_SOURCES
    main.cpp
)

# Çalıştırılabilir dosya oluştur
add_executable(TelgrafApp
    ${PROJECT_SOURCES}
)

# Kütüphaneleri bağla (Qt6::Bluetooth eklendi)
target_link_libraries(TelgrafApp PRIVATE
    Qt6::Widgets
    Qt6::SerialPort
    Qt6::Bluetooth
)

# Windows uygulaması ise konsol penceresini gizle
if(WIN32)
    # Bu satır başındaki # işaretini kaldırdım, artık terminal açılmayacak
    set_target_properties(TelgrafApp PROPERTIES WIN32_EXECUTABLE ON)
endif()

# Mac için Bundle (.app) özelliği ekle
if(APPLE)
    set_target_properties(TelgrafApp PROPERTIES MACOSX_BUNDLE ON)
endif()