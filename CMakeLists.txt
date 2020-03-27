project(cmake_demo)

cmake_minimum_required(VERSION 3.13)

set (CMAKE_CXX_STANDARD 14)
set( CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR} /usr/local/lib/cmake/PahoMqttCpp)
FIND_PACKAGE( Boost 1.67 COMPONENTS program_options thread REQUIRED )
FIND_PACKAGE( Mosquittopp REQUIRED )
FIND_PACKAGE( PahoMqttC REQUIRED )
INCLUDE_DIRECTORIES( ${Boost_INCLUDE_DIR} )

ADD_EXECUTABLE( mcp3427_test mcp3427_test.cpp )
ADD_EXECUTABLE( mcp3427_mqtt mcp3427_mqtt.cpp )

TARGET_LINK_LIBRARIES( mcp3427_mqtt LINK_PUBLIC ${Boost_LIBRARIES} ${MOSQUITTOPP_LIBRARIES} )
TARGET_LINK_LIBRARIES( mcp3427_test LINK_PUBLIC ${Boost_LIBRARIES} )
