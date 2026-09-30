cmake_minimum_required(VERSION 3.24)
project(LibreMaxArchitect VERSION 0.3.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
find_package(Qt6 6.4 REQUIRED COMPONENTS Core Gui Widgets Sql Concurrent Test)
find_package(OpenCASCADE REQUIRED COMPONENTS FoundationClasses ModelingData ModelingAlgorithms Visualization)
find_package(nlohmann_json 3.10 REQUIRED)
find_package(spdlog REQUIRED)
find_package(libzip CONFIG REQUIRED)
add_library(lmx_core
  libs/document/document.cpp libs/document/document.h
  libs/document/examples.cpp libs/document/examples.h
  libs/core/expression.cpp libs/core/expression.h
  libs/import/dxf.cpp libs/import/dxf.h
  libs/materials/texture.cpp libs/materials/texture.h
  libs/commands/editor.cpp libs/commands/editor.h
  libs/geometry/geometry.cpp libs/geometry/geometry.h
  libs/persistence/project_store.cpp libs/persistence/project_store.h
  libs/persistence/recovery_store.cpp libs/persistence/recovery_store.h
  libs/library/library.cpp libs/library/library.h
  libs/library/thumbnails.cpp libs/library/thumbnails.h
  libs/rendering/render_job.cpp libs/rendering/render_job.h)
target_include_directories(lmx_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/libs)
target_include_directories(lmx_core SYSTEM PUBLIC ${OpenCASCADE_INCLUDE_DIR})
target_link_libraries(lmx_core PUBLIC Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Sql Qt6::Concurrent nlohmann_json::nlohmann_json spdlog::spdlog libzip::zip
  TKernel TKMath TKG2d TKG3d TKGeomBase TKBRep TKGeomAlgo TKTopAlgo TKPrim TKBO TKBool TKMesh TKService TKV3d TKOpenGl)
if(MSVC)
  target_compile_options(lmx_core PRIVATE /W4)
else()
  target_compile_options(lmx_core PRIVATE -Wall -Wextra -Wpedantic)
endif()
add_executable(libremax-architect apps/architect/main.cpp apps/architect/main_window.cpp apps/architect/main_window.h
  apps/architect/studio_theme.cpp apps/architect/studio_theme.h libs/viewport/cad_view.cpp libs/viewport/cad_view.h)
target_sources(libremax-architect PRIVATE apps/architect/render_preview.cpp apps/architect/render_preview.h)
qt_add_resources(libremax-architect studio_icons PREFIX "/studio" BASE resources
  FILES resources/arrow-down.png resources/arrow-up.png resources/check.png)
target_link_libraries(libremax-architect PRIVATE lmx_core Qt6::Test)
if(NOT MSVC)
  target_compile_options(libremax-architect PRIVATE -Wall -Wextra -Wpedantic)
endif()
target_compile_definitions(libremax-architect PRIVATE LMX_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
include(CTest)
if(BUILD_TESTING)
  find_package(Catch2 3 QUIET)
  if(NOT Catch2_FOUND)
    find_package(Catch2 2.13 REQUIRED)
  endif()
  add_executable(lmx_tests tests/test_core.cpp)
  target_link_libraries(lmx_tests PRIVATE lmx_core Catch2::Catch2WithMain)
  target_compile_definitions(lmx_tests PRIVATE LMX_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
  add_test(NAME core COMMAND lmx_tests)
  set_tests_properties(core PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
endif()
install(TARGETS libremax-architect RUNTIME DESTINATION bin)
install(FILES resources/libremax-architect.desktop DESTINATION share/applications)
install(FILES resources/libremax.svg DESTINATION share/icons/hicolor/scalable/apps)
install(DIRECTORY starter-library/ DESTINATION share/libremax/starter-library)
install(DIRECTORY starter-materials/ DESTINATION share/libremax/starter-materials)
install(FILES scripts/cycles_render.py DESTINATION share/libremax/scripts)
install(FILES resources/style.qss DESTINATION share/libremax/resources)
install(FILES LICENSE README.md docs/THIRD_PARTY.md DESTINATION share/doc/libremax-architect)
set(CPACK_PACKAGE_NAME libremax-architect)
set(CPACK_PACKAGE_CONTACT "LibreMax contributors")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
set(CPACK_GENERATOR "DEB;TGZ")
include(CPack)
