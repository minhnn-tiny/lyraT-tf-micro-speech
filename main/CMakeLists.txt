
#
# Main component of TF Micro project 'micro_speech'.
#

idf_component_register(
    SRCS esp/main.cc main_functions.cc esp/audio_provider.cc esp/micro_speech_test.cc feature_provider.cc micro_features/no_micro_features_data.cc micro_features/yes_micro_features_data.cc micro_features/model.cc recognize_commands.cc command_responder.cc micro_features/micro_features_generator.cc micro_features/micro_model_settings.cc esp/ringbuf.c esp/lyraT_audio_provider.cc
    INCLUDE_DIRS "")
