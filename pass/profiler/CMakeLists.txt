add_library(RecordDynamicInfo MODULE RecordDynamicInfo.cpp)

target_compile_features(RecordDynamicInfo PRIVATE cxx_range_for cxx_auto_type)
set_target_properties(RecordDynamicInfo PROPERTIES COMPILE_FLAGS "-fno-rtti")
if(APPLE)
    set_target_properties(RecordDynamicInfo PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
endif(APPLE)
