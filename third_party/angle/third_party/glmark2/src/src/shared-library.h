/*
 * Copyright © 2019 Jamie Madill
 *
 * This file is part of the glmark2 OpenGL (ES) 2.0 benchmark.
 *
 * glmark2 is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * glmark2 is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * glmark2.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors:
 *  Jamie Madill
 */
#ifndef GLMARK2_SHARED_LIBRARY_H_
#define GLMARK2_SHARED_LIBRARY_H_

#include <initializer_list>

class SharedLibrary
{
public:
    SharedLibrary();
    ~SharedLibrary();

    bool open(const char *name);
    bool open_from_alternatives(std::initializer_list<const char*> alt_names);
    void close();

    void *handle() const;
    void *load(const char *symbol) const;

private:
    void *handle_;
};

#endif /* GLMARK2_SHARED_LIBRARY_H_ */
