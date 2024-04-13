# copy all the config files in the build dir
file(GLOB CONFIG_INPUTS RELATIVE ${CMAKE_SOURCE_DIR}/config sim* core* DDR*)
foreach(cfile ${CONFIG_INPUTS}) 
  configure_file(${cfile}  ${MOSAIC_CONFIG_DIR}/${cfile})
endforeach() 

# create config.h
configure_file(config.hpp.in ${PROJECT_BINARY_DIR}/include/config.hpp  @ONLY)

# check if Doxygen is installed. If so, create the docs target
if (DOXYGEN_FOUND)
  # set doxygen config file
  set(DOXYGEN_CONFIG ${CMAKE_BINARY_DIR}/docs/config)
  configure_file(doc-config ${DOXYGEN_CONFIG})
  if(Doxygen_dot_FOUND)
    set(FOUND_DOT YES)
  else()
    set(FOUND_DOT NO) 
  endif()
  
  # create the docs target
  add_custom_target( docs
    COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_CONFIG}
    COMMENT "Generating API documentation with Doxygen"
    VERBATIM )
else (DOXYGEN_FOUND)
  message(WARNING "Doxygen not found. The documentaion can not be created.")
endif (DOXYGEN_FOUND)
