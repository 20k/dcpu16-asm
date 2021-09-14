#ifndef HEAP_VECTOR_HPP_INCLUDED
#define HEAP_VECTOR_HPP_INCLUDED

template<typename T>
struct heap_vector
{
    T* svec = nullptr;
    size_t idx = 0;
    size_t current_size = 0;

    constexpr
    heap_vector()
    {

    }

    constexpr
    ~heap_vector()
    {
        delete [] svec;
    }

    constexpr
    void upsize()
    {
        size_t next_size = (current_size + 1) * 2;

        T* next = new T[next_size];

        for(size_t i=0; i < current_size; i++)
        {
            next[i] = svec[i];
        }

        delete [] svec;
        svec = next;

        current_size = next_size;
    }

    constexpr
    void push_back(const T& in)
    {
        if(current_size == idx)
        {
            upsize();
        }

        svec[idx] = in;
        idx++;
    }

    void pop_back()
    {
        idx--;
    }

    constexpr
    T& emplace_back()
    {
        if(current_size == idx)
        {
            upsize();
        }

        svec[idx] = T();

        T& val = svec[idx];

        idx++;

        return val;
    }

    constexpr
    T& operator[](std::size_t idx)
    {
        if(!std::is_constant_evaluated())
        {
            assert(idx < current_size);
        }

        return svec[idx];
    }

    constexpr
    const T& operator[](std::size_t idx) const
    {
        if(!std::is_constant_evaluated())
        {
            assert(idx < current_size);
        }

        return svec[idx];
    }

    constexpr
    size_t size() const
    {
        return idx;
    }

    constexpr
    auto begin()
    {
        return svec;
    }

    constexpr
    auto end()
    {
        if(!std::is_constant_evaluated())
        {
            if(svec == nullptr)
                assert(idx == 0);
        }

        return begin() + idx;
    }

    constexpr
    auto& back()
    {
        if(!std::is_constant_evaluated())
        {
            assert(idx > 0);
            assert(svec);
        }

        return svec[idx - 1];
    }

    constexpr
    const auto& back() const
    {
        if(!std::is_constant_evaluated())
        {
            assert(idx > 0);
            assert(svec);
        }

        return svec[idx - 1];
    }

    constexpr
    void clear()
    {
        idx = 0;
    }
};

#endif // HEAP_VECTOR_HPP_INCLUDED
