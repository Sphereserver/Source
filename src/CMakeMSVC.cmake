# Reset CMake default flags before set flags here to avoid duplicated entries
set(CMAKE_C_FLAGS "")
set(CMAKE_CXX_FLAGS "")

set(ARCH_FLAGS /MP)
set(CODE_GEN_FLAGS /EHa)
set(WARNING_FLAGS /W2)

target_compile_definitions(SphereSvr PUBLIC _WIN32)
target_compile_options(SphereSvr PUBLIC ${ARCH_FLAGS} ${CODE_GEN_FLAGS} ${WARNING_FLAGS})
target_link_libraries(SphereSvr libmysql ws2_32)
set_target_properties(SphereSvr PROPERTIES LINK_FLAGS "/INCREMENTAL:NO /OPT:REF /SUBSYSTEM:WINDOWS")

target_compile_definitions(SphereSvrNightly PUBLIC _NIGHTLYBUILD _WIN32)
target_compile_options(SphereSvrNightly PUBLIC ${ARCH_FLAGS} ${CODE_GEN_FLAGS} ${WARNING_FLAGS})
target_link_libraries(SphereSvrNightly libmysql ws2_32)
set_target_properties(SphereSvrNightly PROPERTIES LINK_FLAGS "/OPT:REF /SUBSYSTEM:WINDOWS")
