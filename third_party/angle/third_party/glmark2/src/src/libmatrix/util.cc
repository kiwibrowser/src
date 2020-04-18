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
#include <sstream>
#include <fstream>
#include <sys/time.h>
#ifdef ANDROID
#include <android/asset_manager.h>
#else
#include <dirent.h>
#endif

#include "log.h"
#include "util.h"

using std::string;
using std::vector;

/*
 * State machine for bash-like quoted string escaping:
 *
 *         \
 *    -----------> +---------+
 *    | ---------- | Escaped |
 *    | |  *,ESC   +---------+
 *    | |
 *    | v      '
 * +--------+ ---> +--------------+ -----
 * | Normal | <--- | SingleQuoted |     | *, ESC
 * +--------+  '   +--------------+ <----
 *    | ^
 *    | |
 *    | |  "       +--------------+ ----
 *    | ---------- | DoubleQuoted |    | *, ESC
 *    -----------> +--------------+ <---
 *         "             | ^
 *                     \ | | *, ESC
 *                       v |
 *             +---------------------+
 *             | DoubleQuotedEscaped |
 *             +---------------------+
 *
 * ESC: Mark character as Escaped
 */
static void
fill_escape_vector(const string &str, vector<bool> &esc_vec)
{
    enum State {
        StateNormal,
        StateEscaped,
        StateDoubleQuoted,
        StateDoubleQuotedEscaped,
        StateSingleQuoted
    };

    State state = StateNormal;

    for (string::const_iterator iter = str.begin();
         iter != str.end();
         iter++)
    {
        const char c(*iter);
        bool esc = false;

        switch (state) {
            case StateNormal:
                if (c == '"')
                    state = StateDoubleQuoted;
                else if (c == '\\')
                    state = StateEscaped;
                else if (c == '\'')
                    state = StateSingleQuoted;
                break;
            case StateEscaped:
                esc = true;
                state = StateNormal;
                break;
            case StateDoubleQuoted:
                if (c == '"')
                    state = StateNormal;
                else if (c == '\\')
                    state = StateDoubleQuotedEscaped;
                else
                    esc = true;
                break;
            case StateDoubleQuotedEscaped:
                esc = true;
                state = StateDoubleQuoted;
                break;
            case StateSingleQuoted:
                if (c == '\'')
                    state = StateNormal;
                else
                    esc = true;
            default:
                break;
        }

        esc_vec.push_back(esc);
    }
}

static void
split_normal(const string& src, char delim, vector<string>& elementVec)
{
    std::stringstream ss(src);
    string item;
    while(std::getline(ss, item, delim))
        elementVec.push_back(item);
}

static void
split_fuzzy(const string& src, char delim, vector<string>& elementVec)
{
    // Fuzzy case: Initialize our delimiter string based upon the caller's plus
    // a space to allow for more flexibility.
    string delimiter(" ");
    delimiter += delim;
    // Starting index into the string of the first token (by definition, if
    // we're parsing a string, there is at least one token).
    string::size_type startPos(0);
    // string::find_first_of() looks for any character in the string provided,
    // it is not treated as a sub-string, so regardless of where the space or
    // comma is or how many there are, the result is the same.
    string str(src);
    string::size_type endPos = str.find_first_of(delimiter);
    while (endPos != string::npos)
    {
        // Push back the current element starting at startPos for
        // (endPos - startPos) characters.  std::string takes care of
        // terminators, etc.
        elementVec.push_back(string(str, startPos, endPos - startPos));
        // Index of the next element after any delimiter characters.  Same
        // caveat applies to find_first_not_of() that applies to
        // find_first_of(); endPos tells it where to start the search. 
        string::size_type nextPos = str.find_first_not_of(delimiter, endPos);
        // Erase the part of the string we've already parsed.
        str = str.erase(startPos, nextPos - startPos);
        // Look for the next delimiter.  If there isn't one, we bail out.
        endPos = str.find_first_of(delimiter);
    }
    // Regardless of whether we initially had one element or many, 'str' now
    // only contains one.
    elementVec.push_back(str);
}

static void
split_quoted(const string& src, char delim, vector<string>& elementVec)
{
    std::stringstream ss;
    vector<bool> escVec;

    /* Mark characters in the string as escaped or not */
    fill_escape_vector(src, escVec);

    /* Sanity check... */
    if (src.length() != escVec.size())
        return;

    for (vector<bool>::const_iterator iter = escVec.begin();
         iter != escVec.end();
         iter++)
    {
        bool escaped = static_cast<bool>(*iter);
        char c = src[iter - escVec.begin()];

        /* Output all characters, except unescaped ",\,' */
        if ((c != '"' && c != '\\' && c != '\'') || escaped) {
            /* If we reach an unescaped delimiter character, do a split */
            if (c == delim && !escaped) {
                elementVec.push_back(ss.str());
                ss.str("");
                ss.clear();
            }
            else {
                ss << c;
            }
        }

    }

    /* Handle final element, delimited by end of string */
    const string &finalElement(ss.str());
    if (!finalElement.empty())
        elementVec.push_back(finalElement);
}

void
Util::split(const string& src, char delim, vector<string>& elementVec,
            Util::SplitMode mode)
{
    // Trivial rejection
    if (src.empty())
    {
        return;
    }

    switch (mode)
    {
        case Util::SplitModeNormal:
            return split_normal(src, delim, elementVec);
        case Util::SplitModeFuzzy:
            return split_fuzzy(src, delim, elementVec);
        case Util::SplitModeQuoted:
            return split_quoted(src, delim, elementVec);
        default:
            break;
    }
}

uint64_t
Util::get_timestamp_us()
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    uint64_t now = static_cast<uint64_t>(ts.tv_sec) * 1000000 +
                   static_cast<double>(ts.tv_nsec)/1000.0;
    return now;
}

std::string
Util::appname_from_path(const std::string& path)
{
    std::string::size_type slashPos = path.rfind("/");
    std::string::size_type startPos(0);
    if (slashPos != std::string::npos)
    {
        startPos = slashPos + 1;
    }
    return std::string(path, startPos, std::string::npos);
}

#ifndef ANDROID

std::istream *
Util::get_resource(const std::string &path)
{
    std::ifstream *ifs = new std::ifstream(path.c_str(), std::ios::binary);

    return static_cast<std::istream *>(ifs);
}

void
Util::list_files(const std::string& dirName, std::vector<std::string>& fileVec)
{
    DIR* dir = opendir(dirName.c_str());
    if (!dir)
    {
        Log::error("Failed to open models directory '%s'\n", dirName.c_str());
        return;
    }

    struct dirent* entry = readdir(dir);
    while (entry)
    {
        std::string pathname(dirName + "/");
        pathname += std::string(entry->d_name);
        // Skip '.' and '..'
        if (entry->d_name[0] != '.')
        {
            fileVec.push_back(pathname);
        }
        entry = readdir(dir);
    }
    closedir(dir);
}

#else

AAssetManager *Util::android_asset_manager = 0;

void
Util::android_set_asset_manager(AAssetManager *asset_manager)
{
    Util::android_asset_manager = asset_manager;
}

AAssetManager *
Util::android_get_asset_manager()
{
    return Util::android_asset_manager;
}

std::istream *
Util::get_resource(const std::string &path)
{
    std::string path2(path);
    /* Remove leading '/' from path name, it confuses the AssetManager */
    if (path2.size() > 0 && path2[0] == '/')
        path2.erase(0, 1);

    std::stringstream *ss = new std::stringstream;
    AAsset *asset = AAssetManager_open(Util::android_asset_manager,
                                       path2.c_str(), AASSET_MODE_RANDOM);
    if (asset) {
        ss->write(reinterpret_cast<const char *>(AAsset_getBuffer(asset)),
                  AAsset_getLength(asset));
        Log::debug("Load asset %s\n", path2.c_str());
        AAsset_close(asset);
    }
    else {
        Log::error("Couldn't load asset %s\n", path2.c_str());
    }

    return static_cast<std::istream *>(ss);
}

void
Util::list_files(const std::string& dirName, std::vector<std::string>& fileVec)
{
    AAssetManager *mgr(Util::android_get_asset_manager());
    std::string dir_name(dirName);

    /* Remove leading '/' from path, it confuses the AssetManager */
    if (dir_name.size() > 0 && dir_name[0] == '/')
        dir_name.erase(0, 1);

    AAssetDir* dir = AAssetManager_openDir(mgr, dir_name.c_str());
    if (!dir)
    {
        Log::error("Failed to open models directory '%s'\n", dir_name.c_str());
        return;
    }

    const char *filename(0);
    while ((filename = AAssetDir_getNextFileName(dir)) != 0)
    {
        std::string pathname(dir_name + "/");
        pathname += std::string(filename);
        fileVec.push_back(pathname);
    }
    AAssetDir_close(dir);
}
#endif
