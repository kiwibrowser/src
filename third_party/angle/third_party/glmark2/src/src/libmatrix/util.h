//
// Copyright (c) 2010-2011 Linaro Limited
//
// All rights reserved. This program and the accompanying materials
// are made available under the terms of the MIT License which accompanies
// this distribution, and is available at
// http://www.opensource.org/licenses/mit-license.php
//
// Contributors:
//     Alexandros Frantzis <alexandros.frantzis@linaro.org>
//     Jesse Barker <jesse.barker@linaro.org>
//
#ifndef UTIL_H_
#define UTIL_H_

#include <string>
#include <vector>
#include <istream>
#include <sstream>
#include <stdint.h>

#ifdef ANDROID
#include <android/asset_manager_jni.h>
#endif

struct Util {
    /**
     * How to perform the split() operation
     */
    enum SplitMode {
        /** Normal split operation */
        SplitModeNormal,
        /** Allow for spaces and multiple consecutive occurences of the delimiter */
        SplitModeFuzzy,
        /** Take into account bash-like quoting and escaping rules */
        SplitModeQuoted
    };

    /**
     * split() - Splits a string into elements using a provided delimiter
     *
     * @s:          the string to split
     * @delim:      the delimiter to use
     * @elems:      the string vector to populate
     * @mode:       the SplitMode to use
     *
     * Using @delim to determine field boundaries, splits @s into separate
     * string elements.  These elements are returned in the string vector
     * @elems. As long as @s is non-empty, there will be at least one
     * element in @elems.
     */
    static void split(const std::string& src, char delim,
                      std::vector<std::string>& elems,
                      Util::SplitMode mode);
    /**
     * get_timestamp_us() - Returns the current time in microseconds
     */
    static uint64_t get_timestamp_us();
    /**
     * get_resource() - Gets an input filestream for a given file.
     *
     * @path:       the path to the file
     *
     * Returns a pointer to an input stream, which must be deleted when no
     * longer in use.
     */
    static std::istream *get_resource(const std::string &path);
    /**
     * list_files() - Get a list of the files in a given directory.
     *
     * @dirName:    the directory path to be listed.
     * @fileVec:    the string vector to populate.
     *
     * Obtains a list of the files in @dirName, and returns them in the string
     * vector @fileVec.
     */
    static void list_files(const std::string& dirName, std::vector<std::string>& fileVec);
    /**
     * dispose_pointer_vector() - cleans up a vector of pointers
     *
     * @vec:        vector of pointers to objects or plain-old-data
     *
     * Iterates across @vec and deletes the data pointed to by each of the
     * elements.  Clears the vector, resetting it for reuse.
     */
    template <class T> static void dispose_pointer_vector(std::vector<T*> &vec)
    {
        for (typename std::vector<T*>::const_iterator iter = vec.begin();
             iter != vec.end();
             iter++)
        {
            delete *iter;
        }

        vec.clear();
    }
    /**
     * toString() - Converts a string to a plain-old-data type.
     *
     * @asString:   a string representation of plain-old-data.
     */
    template<typename T>
    static T
    fromString(const std::string& asString)
    {
        std::stringstream ss(asString);
        T retVal = T();
        ss >> retVal;
        return retVal;
    }
    /**
     * toString() - Converts a plain-old-data type to a string.
     *
     * @t:      a simple value to be converted to a string
     */
    template<typename T>
    static std::string
    toString(const T t)
    {
        std::stringstream ss;
        ss << t;
        return ss.str();
    }
    /**
     * appname_from_path() - get the name of an executable from an absolute path
     *
     * @path:   absolute path of the running application (argv[0])
     *
     * Returns the last portion of @path (everything after the final '/').
     */
    static std::string
    appname_from_path(const std::string& path);

#ifdef ANDROID
    static void android_set_asset_manager(AAssetManager *asset_manager);
    static AAssetManager *android_get_asset_manager(void);
private:
    static AAssetManager *android_asset_manager;
#endif
};

#endif /* UTIL_H */
