#ifndef PtexMutex_h
#define PtexMutex_h

#include <thread>

/*
PTEX SOFTWARE
Copyright 2014 Disney Enterprises, Inc.  All rights reserved

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

  * Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.

  * Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in
    the documentation and/or other materials provided with the
    distribution.

  * The names "Disney", "Walt Disney Pictures", "Walt Disney Animation
    Studios" or the names of its contributors may NOT be used to
    endorse or promote products derived from this software without
    specific prior written permission from Walt Disney Pictures.

Disclaimer: THIS SOFTWARE IS PROVIDED BY WALT DISNEY PICTURES AND
CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE, NONINFRINGEMENT AND TITLE ARE DISCLAIMED.
IN NO EVENT SHALL WALT DISNEY PICTURES, THE COPYRIGHT HOLDER OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND BASED ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
*/

PTEX_NAMESPACE_BEGIN

 /** Automatically acquire and release lock within enclosing scope. */
template <class T>
class AutoLock {
public:
    AutoLock(T& m) : _m(m) { _m.lock(); }
    ~AutoLock()            { _m.unlock(); }
private:
    T& _m;
};

typedef AutoLock<Mutex> AutoMutex;
typedef AutoLock<SpinLock> AutoSpin;

struct RWSpinLock
{
    uint32_t value;
    RWSpinLock() : value(0) {}
    bool TryLock() 
    {
        bool result = AtomicCompareAndSwap(&value, 0u, 0x80000000u) != 0;
        return result;
    }
    void BeginRLock() 
    {
        for (;;)
        {
            uint32_t oldValue = AtomicAdd(&value, 0u) & 0x7fffffffu;
            if (AtomicCompareAndSwap(&value, oldValue, oldValue+1))
            {
                break;
            }
            std::this_thread::yield();
        }
    };
    void EndRLock() 
    {
        AtomicDecrement(&value);
    }
    void BeginWLock() 
    {
        while (!AtomicCompareAndSwap(&value, 0u, 0x80000000u));
    }
    void EndWLock()
    {
        int result = AtomicCompareAndSwap(&value, 0x80000000u, 0u);
        assert(result);
    }
};

struct RWReadLock 
{
    RWSpinLock *lock;
    RWReadLock(RWSpinLock *lock) : lock(lock)
    {
        lock->BeginRLock();
    }
    ~RWReadLock()
    {
        lock->EndRLock();
    }
};

struct RWWriteLock
{
    RWSpinLock *lock;
    RWWriteLock(RWSpinLock *lock) : lock(lock)
    {
        lock->BeginWLock();
    }
    ~RWWriteLock()
    {
        lock->EndWLock();
    }
};

PTEX_NAMESPACE_END

#endif
