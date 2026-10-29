zephyr_library_named(ramenx)
target_include_directories(${ZEPHYR_CURRENT_LIBRARY}
	PUBLIC
		"${CMAKE_CURRENT_LIST_DIR}/include"
		"${CMAKE_CURRENT_LIST_DIR}/ramen"
)
if(CONFIG_RAMENX)
	file(GLOB SOURCES "${CMAKE_CURRENT_LIST_DIR}/src/*.cpp")
	zephyr_library_sources(${SOURCES})
endif()
