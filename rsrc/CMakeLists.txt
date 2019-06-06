# SVG rendering
file(GLOB SVGS ${CMAKE_CURRENT_SOURCE_DIR}/*.svg)
set(PNGS)

foreach(SVG ${SVGS})
    get_filename_component(SVGNAME ${SVG} NAME_WE)
    set(PNG ${SVGNAME}.png)
    message("Adding ${PNG}")
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/${PNG}
        COMMAND rsvg-convert -o ${CMAKE_CURRENT_SOURCE_DIR}/${PNG} ${SVG}
        DEPENDS ${SVG}
    )
    
    list(APPEND PNGS ${PNG})
    
endforeach()

add_custom_target(RSRCS DEPENDS ${PNGS})