# Install script for directory: /Users/stevehiehn/surge-sas/libs/JUCE

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr/local")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Release")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

# Set default install directory permissions.
if(NOT DEFINED CMAKE_OBJDUMP)
  set(CMAKE_OBJDUMP "/Library/Developer/CommandLineTools/usr/bin/objdump")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/Users/stevehiehn/surge-sas/build-arm64/JUCE/modules/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/Users/stevehiehn/surge-sas/build-arm64/JUCE/extras/Build/cmake_install.cmake")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/JUCE-8.0.4" TYPE FILE FILES
    "/Users/stevehiehn/surge-sas/build-arm64/JUCE/JUCEConfigVersion.cmake"
    "/Users/stevehiehn/surge-sas/build-arm64/JUCE/JUCEConfig.cmake"
    "/Users/stevehiehn/surge-sas/libs/JUCE/extras/Build/CMake/JUCECheckAtomic.cmake"
    "/Users/stevehiehn/surge-sas/libs/JUCE/extras/Build/CMake/JUCEHelperTargets.cmake"
    "/Users/stevehiehn/surge-sas/libs/JUCE/extras/Build/CMake/JUCEModuleSupport.cmake"
    "/Users/stevehiehn/surge-sas/libs/JUCE/extras/Build/CMake/JUCEUtils.cmake"
    "/Users/stevehiehn/surge-sas/libs/JUCE/extras/Build/CMake/JuceLV2Defines.h.in"
    "/Users/stevehiehn/surge-sas/libs/JUCE/extras/Build/CMake/LaunchScreen.storyboard"
    "/Users/stevehiehn/surge-sas/libs/JUCE/extras/Build/CMake/PIPAudioProcessor.cpp.in"
    "/Users/stevehiehn/surge-sas/libs/JUCE/extras/Build/CMake/PIPAudioProcessorWithARA.cpp.in"
    "/Users/stevehiehn/surge-sas/libs/JUCE/extras/Build/CMake/PIPComponent.cpp.in"
    "/Users/stevehiehn/surge-sas/libs/JUCE/extras/Build/CMake/PIPConsole.cpp.in"
    "/Users/stevehiehn/surge-sas/libs/JUCE/extras/Build/CMake/RecentFilesMenuTemplate.nib"
    "/Users/stevehiehn/surge-sas/libs/JUCE/extras/Build/CMake/UnityPluginGUIScript.cs.in"
    "/Users/stevehiehn/surge-sas/libs/JUCE/extras/Build/CMake/checkBundleSigning.cmake"
    "/Users/stevehiehn/surge-sas/libs/JUCE/extras/Build/CMake/copyDir.cmake"
    "/Users/stevehiehn/surge-sas/libs/JUCE/extras/Build/CMake/juce_runtime_arch_detection.cpp"
    "/Users/stevehiehn/surge-sas/libs/JUCE/extras/Build/CMake/juce_LinuxSubprocessHelper.cpp"
    )
endif()

