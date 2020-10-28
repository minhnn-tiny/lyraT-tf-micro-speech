
if(NOT DEFINED ENV{IDF_PATH})
  message(FATAL_ERROR "The IDF_PATH environment variable must point to the location of the ESP-IDF.")
endif()


idf_component_register(
  SRCS EspAudioSys.c lock.c MediaHal.c playlistNameDef.c Driver/ES8388_interface.c
  INCLUDE_DIRS . Driver)

# Reduce the level of paranoia to be able to compile TF sources
target_compile_options(${COMPONENT_LIB} PRIVATE
  -Wno-maybe-uninitialized
  -Wno-missing-field-initializers
  -Wno-type-limits)

target_compile_options(${COMPONENT_LIB} PRIVATE -fno-unwind-tables -ffunction-sections -fdata-sections -fmessage-length=0 -DTF_LITE_STATIC_MEMORY -DTF_LITE_DISABLE_X86_NEON -O3 -Wno-nonnull)
target_compile_options(${COMPONENT_LIB} PRIVATE $<$<COMPILE_LANGUAGE:CXX>: -std=c++11 -fno-rtti -fno-exceptions -fno-threadsafe-statics -fno-unwind-tables -ffunction-sections -fdata-sections -fmessage-length=0 -DTF_LITE_STATIC_MEMORY -DTF_LITE_DISABLE_X86_NEON -O3 -Werror -Wsign-compare -Wdouble-promotion -Wshadow -Wunused-variable -Wmissing-field-initializers -Wunused-function -Wswitch -Wvla -Wall -Wextra -Wstrict-aliasing -Wno-unused-parameter  -Wno-return-type -Wno-strict-aliasing -Wno-ignored-qualifiers >)
target_compile_options(${COMPONENT_LIB} INTERFACE $<$<IN_LIST:-DTF_LITE_STATIC_MEMORY,$<TARGET_PROPERTY:${COMPONENT_LIB},COMPILE_OPTIONS>>:-DTF_LITE_STATIC_MEMORY>)
target_link_libraries(${COMPONENT_LIB} PRIVATE -lm)
