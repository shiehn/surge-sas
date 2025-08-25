/* =========================================================================================

   This is an auto-generated file: Any edits you make may be overwritten!

*/

#pragma once

namespace SurgeSharedBinary
{
    extern const char*   configuration_xml;
    const int            configuration_xmlSize = 9090;

    extern const char*   memoryWavetable_wt;
    const int            memoryWavetable_wtSize = 16140;

    extern const char*   oscspecification_html;
    const int            oscspecification_htmlSize = 97591;

    extern const char*   paramdocumentation_xml;
    const int            paramdocumentation_xmlSize = 35303;

    extern const char*   README_UserArea_txt;
    const int            README_UserArea_txtSize = 637;

    extern const char*   windows_wt;
    const int            windows_wtSize = 18444;

    // Number of elements in the namedResourceList and originalFileNames arrays.
    const int namedResourceListSize = 6;

    // Points to the start of a list of resource names.
    extern const char* namedResourceList[];

    // Points to the start of a list of resource filenames.
    extern const char* originalFilenames[];

    // If you provide the name of one of the binary resource variables above, this function will
    // return the corresponding data and its size (or a null pointer if the name isn't found).
    const char* getNamedResource (const char* resourceNameUTF8, int& dataSizeInBytes);

    // If you provide the name of one of the binary resource variables above, this function will
    // return the corresponding original, non-mangled filename (or a null pointer if the name isn't found).
    const char* getNamedResourceOriginalFilename (const char* resourceNameUTF8);
}
