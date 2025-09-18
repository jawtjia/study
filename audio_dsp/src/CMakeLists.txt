set(LIB_NAME dsp_core)

add_library(${LIB_NAME}
  dsp_core/dsp_core.cpp
  dsp_core/hpf.cpp
  dsp_core/stft.cpp
  dsp_core/noise_suppressor.cpp
  common/wav_io.cpp
)

target_include_directories(${LIB_NAME}
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

add_executable(offline_processor offline_processor/main.cpp)
target_link_libraries(offline_processor PRIVATE ${LIB_NAME})


