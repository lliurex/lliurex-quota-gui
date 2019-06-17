cmake_minimum_required(VERSION 3.10)

project(lliurex-quota-gui)

set(APP_NAME lliurex-quota-gui)

# Required Qt settings
find_package(ECM 1.0.0 REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

#Add Qt package
find_package(Qt5 REQUIRED COMPONENTS Widgets Core)

#find_package(XMLRPC REQUIRED c++2)
# XMLRPC find mechanism is broken in this version
# work arround for that
set(XMLRPC_LIBRARIES xmlrpc_client++ xmlrpc_client xmlrpc++ xmlrpc xmlrpc_xmlparse xmlrpc_xmltok xmlrpc_util xmlrpc_packetsocket)

add_subdirectory(rsrc)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(${APP_NAME} main.cpp mainwindow.cpp n4d.cpp resources.qrc mainwindow.ui)
target_link_libraries(${APP_NAME} Qt5::Widgets Qt5::Core ${XMLRPC_LIBRARIES})
add_dependencies(${APP_NAME} RSRCS)

#Install binary
install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/${APP_NAME}
    DESTINATION bin/
)

#desktop
install(
    FILES ${CMAKE_CURRENT_SOURCE_DIR}/lliurex-quota.desktop
    DESTINATION share/applications/
)

#icon
install(
    FILES ${CMAKE_CURRENT_SOURCE_DIR}/rsrc/lliurex-quota.svg
    DESTINATION share/icons/hicolor/scalable/apps/
)

#translations
install(
    DIRECTORY translations
    DESTINATION share/lliurex-quota-gui/
)
