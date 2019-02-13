# Get number of CPU cores to enable multi-thread compiling
execute_process(COMMAND nproc OUTPUT_VARIABLE CPU_CORES)

set(ARCH_FLAGS "-march=i686 -m32")
set(CODE_GEN_FLAGS "-fexceptions -fnon-call-exceptions")
set(GENERAL_FLAGS "-pipe -j${CPU_CORES}")
set(LINKER_FLAGS "-s")
set(OPTIMIZATION_FLAGS "-Os -ffast-math -fno-strict-aliasing -fno-omit-frame-pointer")
set(WARNING_FLAGS "-Wall -Wno-implicit-function-declaration -Wno-invalid-offsetof -Wno-maybe-uninitialized -Wno-switch -Wno-unknown-pragmas -Wno-unused-result")

target_compile_definitions(SphereSvr PUBLIC _MTNETWORK)
target_compile_options(SphereSvr PUBLIC ${ARCH_FLAGS} ${CODE_GEN_FLAGS} ${GENERAL_FLAGS} ${LINKER_FLAGS} ${OPTIMIZATION_FLAGS} ${WARNING_FLAGS})
target_link_libraries(SphereSvr -dynamic dl mysqlclient pthread rt)
set_target_properties(SphereSvr PROPERTIES ENABLE_EXPORTS 0)		# make compiler skip -rdynamic to not include the full symbol table (~700kb) on compiled file

target_compile_definitions(SphereSvrNightly PUBLIC _NIGHTLYBUILD _MTNETWORK)
target_compile_options(SphereSvr PUBLIC ${ARCH_FLAGS} ${CODE_GEN_FLAGS} ${GENERAL_FLAGS} ${LINKER_FLAGS} ${OPTIMIZATION_FLAGS} ${WARNING_FLAGS})
target_link_libraries(SphereSvrNightly -dynamic dl mysqlclient pthread rt)
set_target_properties(SphereSvrNightly PROPERTIES ENABLE_EXPORTS 0)		# make compiler skip -rdynamic to not include the full symbol table (~700kb) on compiled file
