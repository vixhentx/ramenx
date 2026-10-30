cmake_minimum_required(VERSION 3.20.0)

if(NOT BOARD)
    set(BOARD native_sim/native/64)
    message("BOARD not defined, use default value: ${BOARD}")
endif()

list(APPEND ZEPHYR_EXTRA_MODULES ${CMAKE_CURRENT_LIST_DIR}/..)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(ramenx_test_app)

file(GLOB SOURCES "${CMAKE_CURRENT_LIST_DIR}/src/*.cpp")
target_include_directories(app
	PRIVATE
		"${CMAKE_CURRENT_LIST_DIR}/inc"
)
target_sources(app
	PRIVATE
		${SOURCES}
)
target_link_libraries(app
	PRIVATE
		ramenx
)
