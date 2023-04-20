# TODO(useredsa): Are the LTO and the compile options meant to be set here?

add_library(mbp_core SHARED core/predictor.cpp core/branch.cpp)
target_include_directories(mbp_core PUBLIC ../include)
set_target_properties(mbp_core PROPERTIES
  INTERPROCEDURAL_OPTIMIZATION TRUE
)
target_compile_features(mbp_core PUBLIC cxx_std_17)
target_compile_options(mbp_core PRIVATE
  "-Wall" "-O3" "-march=native" "-mtune=native"
)

add_library(mbp_trace_reader SHARED sim/sbbt_reader.cpp)
target_link_libraries(mbp_trace_reader PUBLIC mbp_core)
target_include_directories(mbp_trace_reader PUBLIC ../include)
set_target_properties(mbp_trace_reader PROPERTIES
  INTERPROCEDURAL_OPTIMIZATION TRUE
)
target_compile_features(mbp_trace_reader PUBLIC cxx_std_17)
target_compile_options(mbp_trace_reader PRIVATE
  "-Wall" "-O3" "-march=native" "-mtune=native"
)

add_library(mbp_sbbt_writer SHARED sim/sbbt_writer.cpp)
target_include_directories(mbp_sbbt_writer PUBLIC ../include)
set_target_properties(mbp_sbbt_writer PROPERTIES
  INTERPROCEDURAL_OPTIMIZATION TRUE
)
target_compile_features(mbp_sbbt_writer PUBLIC cxx_std_17)
target_compile_options(mbp_sbbt_writer PRIVATE
  "-Wall" "-O3" "-march=native" "-mtune=native"
)

add_library(mbp_sim SHARED sim/simulator.cpp)
target_link_libraries(mbp_sim PUBLIC mbp_core PRIVATE mbp_trace_reader)
target_include_directories(mbp_sim PUBLIC ../include)
set_target_properties(mbp_sim PROPERTIES
  INTERPROCEDURAL_OPTIMIZATION TRUE
)
target_compile_features(mbp_sim PUBLIC cxx_std_17)
target_compile_options(mbp_sim PRIVATE
  "-Wall" "-O3" "-march=native" "-mtune=native"
)

add_library(mbp_utils INTERFACE)
target_include_directories(mbp_utils INTERFACE ../include)
target_compile_features(mbp_utils INTERFACE cxx_std_17)

add_library(mbp_examples INTERFACE)
target_link_libraries(mbp_examples INTERFACE mbp_core mbp_utils)
target_include_directories(mbp_examples INTERFACE ../include)
set_target_properties(mbp_examples PROPERTIES
  INTERPROCEDURAL_OPTIMIZATION TRUE
)
target_compile_features(mbp_examples INTERFACE cxx_std_17)
target_compile_options(mbp_examples INTERFACE
  "-Wall" "-O3" "-march=native" "-mtune=native"
)
