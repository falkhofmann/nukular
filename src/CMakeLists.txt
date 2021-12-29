set(PLUGINS
    Circle
    CircularRamp
    CircularRays
    CircularRings
    Kontrast
    Scroll
    Vibrant
    )

# Python Menu
set(DRAW_NODES Circle CircularRamp CircularRays CircularRings)
set(COLOR_NODES Kontrast Vibrant)
set(TRANSFORM_NODES Scroll)

set(BLINK_NODES Clarity)

# add nuke plugin linked to ddimage lib
function(add_nuke_plugin PLUGIN_NAME)
    add_library(${PLUGIN_NAME} MODULE ${ARGN})
    add_library(NukePlugins::${PLUGIN_NAME} ALIAS ${PLUGIN_NAME})
    target_link_libraries(${PLUGIN_NAME} PRIVATE ${NUKE_DDIMAGE_LIBRARY})
    set_target_properties(${PLUGIN_NAME} PROPERTIES PREFIX "")
    if (APPLE)
        set_target_properties(${PLUGIN_NAME} PROPERTIES SUFFIX ".dylib")
    endif()
endfunction()

# include directories
include_directories(${NUKE_INCLUDE_DIRS})

# add configuration 
foreach(PLUGIN_NAME ${PLUGINS})
    add_nuke_plugin(${PLUGIN_NAME} ${PLUGIN_NAME}.cpp)
endforeach()

# create menu file 
string(REPLACE ";" "\", \"" DRAW_NODES "\"${DRAW_NODES}\"")
string(REPLACE ";" "\", \"" COLOR_NODES "\"${COLOR_NODES}\"")
string(REPLACE ";" "\", \"" FILTER_NODES "\"${FILTER_NODES}\"")
string(REPLACE ";" "\", \"" TRANSFORM_NODES "\"${TRANSFORM_NODES}\"")

string(REPLACE ";" "\", \"" BLINK_NODES "\"${BLINK_NODES}\"")


configure_file(../python/menu.py.in menu.py)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/menu.py" DESTINATION .)

# install files
install(TARGETS 
        ${PLUGINS} 
        DESTINATION .)
