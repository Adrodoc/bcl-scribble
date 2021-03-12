#pragma once

class Lock
{
public:
    virtual void acquire() = 0;
    virtual void release() = 0;
};
