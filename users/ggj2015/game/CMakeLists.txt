cmake_minimum_required(VERSION 2.6)

set(CMAKE_CXX_STANDARD 14)

set(HAS_LIBGG 0)
set(HAS_FRAMEWORK 0)

# --- dependencies ---

if (NOT HAS_LIBGG)
	message("including libgg")
	add_subdirectory(../../../libgg libgg)
endif (NOT HAS_LIBGG)
if (NOT HAS_FRAMEWORK)
	message("including framework")
	add_subdirectory(../../../framework framework)
endif (NOT HAS_FRAMEWORK)

#

project(ggj2015)

# --- game ---

file(GLOB source "*.cpp" "*.h")

source_group("sources" FILES ${source})

add_executable(game ${source})

target_include_directories(game PRIVATE "${PROJECT_SOURCE_DIR}")

target_link_libraries(game PRIVATE framework)
target_link_libraries(game PRIVATE libgg)

target_link_libraries(game PRIVATE ${FRAMEWORK_LIBRARIES})

if (APPLE)
	target_compile_definitions(game PUBLIC MACOS)
endif (APPLE)

if (MSVC)
	set_property(TARGET game APPEND_STRING PROPERTY COMPILE_FLAGS /wd4244)
endif (MSVC)
