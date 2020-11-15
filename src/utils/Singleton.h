//========================================================================
//
// Copyright (C) 2018 Matthieu Bruel <Matthieu.Bruel@gmail.com>
//
// This file is a part of ClementineRemote : https://github.com/mbruel/miniEMF
//
// ClementineRemote is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as
// published by the Free Software Foundation; version 3.0 of the License.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// You should have received a copy of the GNU Lesser General Public
// License along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301,
// USA.
//
//========================================================================

#ifndef SINGLETON_H
#define SINGLETON_H

template <typename T> class Singleton
{
protected:
    // Constructor /Destructor
    Singleton() = default;
    virtual ~Singleton() = default;

public:
    Singleton(const Singleton &other) = delete;
    Singleton(const Singleton &&other) = delete;
    Singleton & operator=(const Singleton &other) = delete;
    Singleton & operator=(const Singleton &&other) = delete;

    // Public Interface
    static T *getInstance(){
        if (!_singleton) {
            _singleton = new T;
        }
        return (static_cast<T*> (_singleton));
    }

    static void kill()
    {
        if (_singleton)
        {
            delete _singleton;
            _singleton = nullptr;
        }
    }

private:
    // Unique instance
    static T *_singleton;
};

template <typename T> T *Singleton<T>::_singleton;
#endif // SINGLETON_H
