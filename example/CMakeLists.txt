cmake_minimum_required(VERSION 3.10)

project(MBPlib_examples
  VERSION 0.3.0
  DESCRIPTION "Branch predictors and cmake integration example for MBPlib"
  LANGUAGES CXX
)

# Add this line (with the proper path) to link aggainst MBPlib
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../
  ${CMAKE_CURRENT_SOURCE_DIR}/../build)

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
  set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)
  set(CMAKE_CXX_EXTENSIONS OFF)
endif()

# Function for adding a predictor simulator as CMake target.
#
# @param name Executable name.
# @param predictor Instantiation of the predictor object.
# @param extra_args Source files to compile with the target.
function(add_mbp_sim name predictor)
  add_executable(${name} src/main.cpp ${ARGN})
  target_compile_definitions(${name} PRIVATE PREDICTOR=${predictor})
  target_link_libraries(${name} PRIVATE mbp_examples mbp_sim)
  set_target_properties(${name} PROPERTIES
    CXX_STANDARD 17
    INTERPROCEDURAL_OPTIMIZATION TRUE
  )
  target_compile_options(${name} PRIVATE
    "-Wall" "-O3" "-march=native" "-mtune=native"
  )
endfunction()

# Function for adding a predictor comparison simulator as CMake target.
#
# @param name Executable name.
# @param predictor0 Instantiation of the first predictor object.
# @param predictor1 Instantiation of the second predictor object.
# @param extra_args Source files to compile with the target.
function(add_mbp_comp name predictor0 predictor1)
  add_executable(${name} src/compare.cpp ${ARGN})
  target_compile_definitions(${name} PRIVATE PREDICTOR0=${predictor0})
  target_compile_definitions(${name} PRIVATE PREDICTOR1=${predictor1})
  target_link_libraries(${name} PRIVATE mbp_examples mbp_sim)
  set_target_properties(${name} PROPERTIES
    CXX_STANDARD 17
    INTERPROCEDURAL_OPTIMIZATION TRUE
  )
  target_compile_options(${name} PRIVATE
    "-Wall" "-O3" "-march=native" "-mtune=native"
  )
endfunction()

# Historical Single Component Predictors
# Gshare is considered the best single component predictor.
add_mbp_sim(bimodal_64KB "mbp::Bimodal<18>{}")
add_mbp_sim(nmodal_3_64KB "mbp::Nmodal<3, 17>{}")
add_mbp_sim(nmodal_4_64KB "mbp::Nmodal<4, 17>{}")
add_mbp_sim(gshare_32KB "mbp::Gshare<25, 17>{}")
add_mbp_sim(gshare_64KB "mbp::Gshare<25, 18>{}")

# Two Level Predictors
# Best parameters for the SPEC95 suite.
# Most performant (for the same suite) is GAs.
add_mbp_sim(gag_64KB "mbp::TwoLevel<18, 0, 0, 0, 0>{}")
add_mbp_sim(gap_64KB "mbp::TwoLevel<15, 0, 0, 3, 0>{}")
add_mbp_sim(gas_64KB "mbp::TwoLevel<15, 0, 0, 3, 3>{}")
add_mbp_sim(pag_64KB "mbp::TwoLevel<18, 13, 0, 0, 0>{}")
add_mbp_sim(pap_64KB "mbp::TwoLevel<13, 1, 0, 4, 0>{}")
add_mbp_sim(pas_64KB "mbp::TwoLevel<13, 1, 0, 4, 3>{}")
add_mbp_sim(sag_64KB "mbp::TwoLevel<18, 1, 12, 0, 0>{}")
add_mbp_sim(sap_64KB "mbp::TwoLevel<13, 1, 12, 4, 0>{}")
add_mbp_sim(sas_64KB "mbp::TwoLevel<12, 1, 12, 4, 5>{}")

# Historical Multiple Component Predictors
add_mbp_sim(tnmnt_gshare_bim_64KB
    "mbp::BimodalTournament<mbp::Gshare<25, 17>, mbp::Bimodal<16>, 16>{}")
add_mbp_sim(gtnmnt_gshare_bim_64KB
    "mbp::GshareTournament<mbp::Gshare<25, 17>, mbp::Bimodal<16>, 20, 16>{}")
# Another way of obtaining the same predictor
add_mbp_sim(gtnmnt_gshare_bim_alt "mbp::TournamentPred{         \
    std::unique_ptr<mbp::Predictor>(new mbp::Gshare<20, 16>{}), \
    std::unique_ptr<mbp::Predictor>(new mbp::Bimodal<16>{}),    \
    std::unique_ptr<mbp::Predictor>(new mbp::Gshare<25, 17>{})  \
}")
add_mbp_sim(2bcgskew_64KB "mbp::Twobcgskew<>{}")

# State of the Art Predictors
add_mbp_sim(hashed_perceptron_64KB "mbp::HashedPerceptron<4, 16, 12, 18>{}")
add_mbp_sim(tage_64KB "mbp::Tage{{TAGE_SPECS.begin(), TAGE_SPECS.end()}}")
add_mbp_sim(batage_64KB
    "mbp::Batage{{BATAGE_SPECS.begin(), BATAGE_SPECS.end()}, BATAGE_SEED}")
add_mbp_sim(tage_sc_l_64KB "mbp::TageScL{}")

# Example comparing 2bcgskew_64KB and gshare_64KB
add_mbp_comp(2bcgskwew_vs_gshare_64KB
  "mbp::Twobcgskew<>{}" "mbp::Gshare<25, 18>{}")

# Example evaluating multiple batages simultaneously
foreach(num RANGE 2 20 6)
  add_executable(parallel_sim_${num} src/parallel_simulation_example.cpp)
  target_compile_definitions(parallel_sim_${num}
	PRIVATE
      NUM_PREDICTORS_TESTED=${num}
  )
  target_link_libraries(parallel_sim_${num} PRIVATE mbp_examples mbp_utils mbp_sim)
  set_target_properties(parallel_sim_${num} PROPERTIES
    CXX_STANDARD 17
    INTERPROCEDURAL_OPTIMIZATION TRUE
  )
  target_compile_options(parallel_sim_${num} PRIVATE
    "-Wall" "-O3" "-march=native" "-mtune=native"
  )
endforeach()
