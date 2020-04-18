#FIXME in the future: with cmake 2.8.12 and up it's better
#to use target_compile_options or add_compile_options
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -fvisibility=hidden -fPIC")
set(CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS} -std=c99 -Wall")

# Make directory for generated parser files
set(PARSER_GEN_DIR "${CMAKE_CURRENT_SOURCE_DIR}/gen")
file(MAKE_DIRECTORY ${PARSER_GEN_DIR})

include_directories ("${PROJECT_SOURCE_DIR}" "${PARSER_GEN_DIR}" )

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(PARSER_DEBUG_OPTIONS "--debug -v")
endif (CMAKE_BUILD_TYPE STREQUAL "Debug")

find_package(BISON REQUIRED)
BISON_TARGET(Parser parser.ypp ${PARSER_GEN_DIR}/parser.cpp COMPILE_FLAGS "-l -p wds_ ${PARSER_DEBUG_OPTIONS}")

find_package(FLEX REQUIRED)
FLEX_TARGET(MessageLexer messagelexer.l ${PARSER_GEN_DIR}/messagescanner.cpp COMPILE_FLAGS "-L -P message_ ${PARSER_DEBUG_OPTIONS}")
ADD_FLEX_BISON_DEPENDENCY(MessageLexer Parser)
FLEX_TARGET(HeaderLexer headerlexer.l ${PARSER_GEN_DIR}/headerscanner.cpp COMPILE_FLAGS "-L -P header_ ${PARSER_DEBUG_OPTIONS}")
ADD_FLEX_BISON_DEPENDENCY(HeaderLexer Parser)
FLEX_TARGET(ErrorLexer errorlexer.l ${PARSER_GEN_DIR}/errorscanner.cpp COMPILE_FLAGS "-L -P error_ ${PARSER_DEBUG_OPTIONS}")
ADD_FLEX_BISON_DEPENDENCY(ErrorLexer Parser)

add_library(wdsrtsp OBJECT
    ${BISON_Parser_OUTPUT_SOURCE}
    ${FLEX_MessageLexer_OUTPUTS}
    ${FLEX_ErrorLexer_OUTPUTS}
    ${FLEX_HeaderLexer_OUTPUTS}
    driver.cpp message.cpp header.cpp transportheader.cpp payload.cpp
    options.cpp reply.cpp getparameter.cpp setparameter.cpp play.cpp
    pause.cpp teardown.cpp setup.cpp property.cpp genericproperty.cpp
    formats3d.cpp audiocodecs.cpp clientrtpports.cpp
    contentprotection.cpp coupledsink.cpp displayedid.cpp
    presentationurl.cpp route.cpp triggermethod.cpp
    videoformats.cpp i2c.cpp avformatchangetiming.cpp uibcsetting.cpp
    standbyresumecapability.cpp standby.cpp idrrequest.cpp connectortype.cpp
    preferreddisplaymode.cpp uibccapability.cpp propertyerrors.cpp
)
