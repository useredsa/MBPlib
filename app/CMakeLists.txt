add_executable(sbbt_cat sbbt_cat/main.cpp)
target_link_libraries(sbbt_cat PRIVATE mbp_trace_reader)
set_target_properties(sbbt_cat PROPERTIES
  CXX_STANDARD 17
  CXX_EXTENSIONS OFF
  INTERPROCEDURAL_OPTIMIZATION TRUE
)
target_include_directories(sbbt_cat PRIVATE include)
target_compile_options(sbbt_cat PRIVATE
  "-Wall" "-O3" "-march=native" "-mtune=native"
)

add_executable(sbbt_inspect sbbt_inspect/main.cpp)
target_link_libraries(sbbt_inspect PRIVATE mbp_trace_reader)
set_target_properties(sbbt_inspect PROPERTIES
  CXX_STANDARD 17
  CXX_EXTENSIONS OFF
  INTERPROCEDURAL_OPTIMIZATION TRUE
)
target_include_directories(sbbt_inspect PRIVATE include)
target_compile_options(sbbt_inspect PRIVATE
  "-Wall" "-O3" "-march=native" "-mtune=native"
)
