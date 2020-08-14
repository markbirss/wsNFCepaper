cmake_minimum_required(VERSION 3.13)

project(epnfcup)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/Modules)

set(VERSION "0.0.1")

ADD_EXECUTABLE(epnfcup epnfcup.c)

find_package(LIBNFC REQUIRED)
find_package(ImageMagick COMPONENTS MagickWand)

include_directories(
	${LIBNFC_INCLUDE_DIRS}
	${ImageMagick_INCLUDE_DIRS}
)

link_directories(
	${LIBNFC_LIBRARIES}
	${ImageMagick_LIBRARIES}
)

# get MAGICKCORE_HDRI_ENABLE and MAGICKCORE_QUANTUM_DEPTH cflags from MagickWand-config
if(ImageMagick_FOUND)
    # Find Imagemagick Library directory
    get_filename_component(MAGICK_LIB_DIR ${ImageMagick_MagickWand_LIBRARY} DIRECTORY)
    # Find where Magick++-config lives
    file(GLOB_RECURSE MAGICK_CONFIG FOLLOW_SYMLINKS ${MAGICK_LIB_DIR}/MagickWand-config)
    # Ask about CXX and lib flags/locations
    set(MAGICK_CONFIG ${MAGICK_CONFIG} CACHE string "Path to Magick-config utility")
    execute_process(COMMAND "${MAGICK_CONFIG}" "--cflags" OUTPUT_VARIABLE MAGICK_CXX_FLAGS)
    # Split into list:
    string(REGEX MATCHALL "([^\ ]+)" MAGICK_CXX_FLAGS "${MAGICK_CXX_FLAGS}")
    # Remove trailing whitespace (CMAKE warns about this)
    string(STRIP "${MAGICK_CXX_FLAGS}" MAGICK_CXX_FLAGS)
    # Add these to cache
    set(MAGICK_CXX_FLAGS "${MAGICK_CXX_FLAGS}" CACHE string "ImageMagick configuration specific compilation flags." )

    target_compile_options(epnfcup PUBLIC ${MAGICK_CXX_FLAGS})
endif(ImageMagick_FOUND)

target_link_libraries(
	epnfcup
	${LIBNFC_LIBRARIES}
	${ImageMagick_LIBRARIES}
)

install(TARGETS epnfcup DESTINATION /usr/bin)

set(CPACK_GENERATOR "DEB")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Denis Bodor")
set(CPACK_PACKAGE_VERSION "${VERSION}")
set(CPACK_DEBIAN_PACKAGE_DESCRIPTION "Waveshare NFC E-ink upload tool")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libc6, libmagickwand-6.q16-6, libnfc5")
include(CPack)

