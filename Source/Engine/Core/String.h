#pragma once
#include <string>

namespace flower::string
{
    inline bool endWith(std::string const& fullString, std::string const& ending) 
    {
        if (fullString.length() >= ending.length()) 
        {
            return (0 == fullString.compare(fullString.length() - ending.length(), ending.length(), ending));
        } 
        else 
        {
            return false;
        }
    }

    // remove all char behind last "." also include "." itself.
    // eg: ./Content/Mesh/A.obj -> ./Content/Mesh/A
    inline std::string getFileRawName(const std::string& pathStr)
    {
        return pathStr.substr(0, pathStr.rfind("."));
    }
}