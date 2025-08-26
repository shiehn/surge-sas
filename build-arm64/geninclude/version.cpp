/*
** This file is rebuilt and substitited every time you run a build.
** Things which need to be per-build should be defined here, declared
** in the version.h header, and then used wherever you want
*/
#include "version.h"

// clang-format off
namespace Surge
{
   const char* Build::MajorVersionStr = "1";
   const int   Build::MajorVersionInt = 1;
   
   const char* Build::SubVersionStr = "4";
   const int   Build::SubVersionInt = 4;
   
   const char* Build::ReleaseNumberStr = "1000";
   const char* Build::ReleaseStr = "feat/tcp-control-api";

   const bool Build::IsRelease = 0;
   const bool Build::IsNightly = ! Build::IsRelease;

   const char* Build::BuildNumberStr = "f42be6fd"; // Build number to be sure that each result could identified.
   
   const char* Build::FullVersionStr = "1.4.feat/tcp-control-api.f42be6fd";
   const char* Build::BuildHost = "Steves-Mac-mini.local";
   const char* Build::BuildArch = "arm64";
   const char *Build::BuildCompiler = "AppleClang-14.0.3.14030022";

   const char* Build::BuildLocation = "local";

   const char* Build::BuildDate = "2025-08-25";
   const char* Build::BuildTime = "18:50:12";
   const char* Build::BuildYear = "2025";

   const char* Build::GitHash = "f42be6fd";
   const char* Build::GitBranch = "feat/tcp-control-api";

   const char* Build::CMAKE_INSTALL_PREFIX = "/usr/local";
}
// clang-format on
