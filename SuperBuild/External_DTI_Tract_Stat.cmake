set(DTI_Tract_Stat_DEPENDS QWT ${ITK_EXTERNAL_NAME} VTK SlicerExecutionModel)

ExternalProject_Add(DTI_Tract_Stat
  SVN_REPOSITORY "https://www.nitrc.org/svn/dti_tract_stat/trunk"
  SVN_USERNAME slicerbot
  SVN_PASSWORD slicer
  SOURCE_DIR DTI_Tract_Stat
  BINARY_DIR DTI_Tract_Stat-build
  ${cmakeversion_external_update} "${cmakeversion_external_update_value}"
  DEPENDS ${DTI_Tract_Stat_DEPENDS}
  CMAKE_GENERATOR ${gen}
##  PATCH_COMMAND ${CMAKE_COMMAND}
##  -Dfixfile=${CMAKE_CURRENT_BINARY_DIR}/DTI_Tract_Stat/CMakeLists.txt
##  -P ${CMAKE_CURRENT_LIST_DIR}/DTI_Tract_Stat_Fix.cmake
  CMAKE_ARGS
    --no-warn-unused-cli # HACK Only expected variables should be passed down.
    -DQWT_INCLUDE_DIR:PATH=${QWT_INCLUDE_DIR}
    -DQWT_LIBRARY:PATH=${QWT_LIBRARY}
  ${CMAKE_OSX_EXTERNAL_PROJECT_ARGS}
  ${COMMON_EXTERNAL_PROJECT_ARGS}
  INSTALL_COMMAND ""
  INSTALL_DIR ${CMAKE_INSTALL_PREFIX}
  )
