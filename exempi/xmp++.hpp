/*
 *
 * Copyright (C) 2011 Hubert Figuiere
 *
 *  Distributed under the Boost Software License, Version 1.0. (See
 *  accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 */

#ifndef __XMP_PLUSPLUS_H__
#define __XMP_PLUSPLUS_H__

#include <exempi/xmp.h>

namespace xmp {

inline void release(XmpIteratorPtr ptr)
{
    xmp_iterator_free(ptr);
}

inline void release(XmpStringPtr ptr)
{
    xmp_string_free(ptr);
}

inline void release(XmpFilePtr ptr)
{
    xmp_files_free(ptr);
}

inline void release(XmpPtr ptr)
{
    xmp_free(ptr);
}

/**
 * @brief a scoped pointer for Xmp opaque types
 */
template <class T>
class ScopedPtr {
public:
    ScopedPtr(T p) : _p(p) {}
    ~ScopedPtr()
    {
        if (_p)
            release(_p);
    }
    operator T() const { return _p; }

private:
    T _p;

    // private copy constructor and assignment
    // to make the class non copiable
    ScopedPtr(const ScopedPtr<T> &);
    ScopedPtr &operator=(const ScopedPtr<T> &);
};
}

#endif
