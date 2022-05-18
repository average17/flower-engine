#pragma once
#include <atomic>
#include <cstdint>

namespace flower 
{
    template<class T>
    class RefCounter : public T
    {
    private:
        std::atomic<uint64_t> m_refCount = 1;

    public:
        virtual uint64_t addRef() override
        {
            return ++m_refCount;
        }

        virtual uint64_t release() override
        {
            uint64_t result = --m_refCount;

            if (result == 0) 
            {
                delete this;
            }
            return result;
        }
    };

}