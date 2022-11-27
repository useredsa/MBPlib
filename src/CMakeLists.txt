# TODO(useredsa): Are the LTO and the compile options meant to be set here?

add_library(mbp_trace_reader SHARED sim/sbbt_reader.cpp sim/branch.cpp)
target_include_directories(mbp_trace_reader PUBLIC ../include)
set_target_properties(mbp_trace_reader PROPERTIES
  INTERPROCEDURAL_OPTIMIZATION TRUE
)
target_compile_features(mbp_trace_reader PUBLIC cxx_std_17)
target_compile_options(mbp_trace_reader PRIVATE
  "-Wall" "-O3" "-march=native" "-mtune=native"
)

add_library(mbp_sim SHARED sim/simulator.cpp sim/predictor.cpp)
target_link_libraries(mbp_sim PRIVATE mbp_trace_reader)
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

add_library(mbp_examples SHARED examples/tage_sc_l_cbp5.cpp)
target_include_directories(mbp_examples PUBLIC ../include)
set_target_properties(mbp_examples PROPERTIES
  INTERPROCEDURAL_OPTIMIZATION TRUE
)
target_compile_features(mbp_examples PUBLIC cxx_std_17)
target_compile_options(mbp_examples PRIVATE
  "-Wall" "-O3" "-march=native" "-mtune=native"
)
