include("C:/Qt/Project_Repos/TestLeadToolbox/build/Desktop_Qt_6_10_1_MinGW_64_bit-Debug/.qt/QtDeploySupport.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/TestLeadToolbox-plugins.cmake" OPTIONAL)
set(__QT_DEPLOY_I18N_CATALOGS "qtbase")

qt6_deploy_runtime_dependencies(
    EXECUTABLE "C:/Qt/Project_Repos/TestLeadToolbox/build/Desktop_Qt_6_10_1_MinGW_64_bit-Debug/TestLeadToolbox.exe"
    GENERATE_QT_CONF
)
