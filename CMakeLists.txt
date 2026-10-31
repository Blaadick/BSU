cmake_minimum_required(VERSION 4.4)
set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "f35a9ac6-8463-4d38-8eec-5d6008153e7d")

project(BSU
    DESCRIPTION "Blaad's System Utilities"
    VERSION 0.1.1
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_MODULE_STD ON)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)

include(GenerateExportHeader)
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

add_library(bsu SHARED)
target_compile_features(bsu PUBLIC cxx_std_26)
set_target_properties(bsu PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)

target_sources(bsu
    PUBLIC FILE_SET CXX_MODULES BASE_DIRS include FILES
    include/bsu.cppm
    include/filesystem.cppm
    include/secrets.cppm
    PRIVATE
    src/filesystem.cpp
    src/secrets.cpp
)

if(NOT WIN32)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(LibSecret REQUIRED IMPORTED_TARGET libsecret-1)
    target_link_libraries(bsu PRIVATE $<BUILD_INTERFACE:PkgConfig::LibSecret>)
endif()

target_include_directories(bsu PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/bsu>
)

generate_export_header(bsu EXPORT_FILE_NAME ${CMAKE_CURRENT_BINARY_DIR}/include/bsu.hpp)

install(TARGETS bsu EXPORT BSUTargets
    FILE_SET CXX_MODULES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/bsu
)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/include/bsu.hpp DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/bsu)
install(FILES LICENSE DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/licenses/bsu)
install(EXPORT BSUTargets
    NAMESPACE BSU::
    FILE BSUConfig.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/BSU
)

write_basic_package_version_file(${CMAKE_CURRENT_BINARY_DIR}/BSUConfigVersion.cmake COMPATIBILITY SameMajorVersion)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/BSUConfigVersion.cmake DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/BSU)
