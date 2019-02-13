# Get number of CPU cores to enable multi-thread compiling
execute_process(COMMAND cmd /c "echo %NUMBER_OF_PROCESSORS%" OUTPUT_VARIABLE CPU_CORES)

set(ARCH_FLAGS "-march=i686 -m32")
set(CODE_GEN_FLAGS "-fexceptions -fnon-call-exceptions")
set(GENERAL_FLAGS "-pipe -j${CPU_CORES}")
set(LINKER_FLAGS "-s")
set(OPTIMIZATION_FLAGS "-Os -ffast-math -fno-strict-aliasing -fno-omit-frame-pointer")
set(WARNING_FLAGS "-Wall -Wno-implicit-function-declaration -Wno-invalid-offsetof -Wno-maybe-uninitialized -Wno-switch -Wno-unknown-pragmas -Wno-unused-result")

target_compile_definitions(SphereSvr PUBLIC _MTNETWORK _WIN32)
target_compile_options(SphereSvr PUBLIC ${ARCH_FLAGS} ${CODE_GEN_FLAGS} ${GENERAL_FLAGS} ${LINKER_FLAGS} ${OPTIMIZATION_FLAGS} ${WARNING_FLAGS})
target_link_libraries(SphereSvr -dynamic -static-libgcc -static-libstdc++ libmysql ws2_32)

target_compile_definitions(SphereSvrNightly PUBLIC _NIGHTLYBUILD _MTNETWORK _WIN32)
target_compile_options(SphereSvrNightly PUBLIC ${ARCH_FLAGS} ${CODE_GEN_FLAGS} ${GENERAL_FLAGS} ${LINKER_FLAGS} ${OPTIMIZATION_FLAGS} ${WARNING_FLAGS})
target_link_libraries(SphereSvrNightly -dynamic -static-libgcc -static-libstdc++ libmysql ws2_32)
