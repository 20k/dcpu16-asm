#ifndef HEAP_VECTOR_HPP_INCLUDED
#define HEAP_VECTOR_HPP_INCLUDED

template<typename T>
struct heap_vector
{
    T* data = nullptr;
    size_t idx = 0;
    size_t current_size = 0;

    constexpr
    heap_vector()
    {

    }

    constexpr
    heap_vector(const heap_vector<T>& other)
    {
        idx = other.idx;
        current_size = other.current_size;

        data = new T[current_size];

        for(size_t i=0; i < idx; i++)
        {
            data[i] = other.data[i];
        }
    }

    constexpr
    heap_vector<T>& operator=(const heap_vector<T>& other)
    {
        return *this = heap_vector<T>(other);
    }

    constexpr
    ~heap_vector()
    {
        delete [] data;
    }

    constexpr
    void upsize()
    {
        size_t next_size = (current_size + 1) * 2;

        T* next = new T[next_size];

        for(size_t i=0; i < current_size; i++)
        {
            next[i] = data[i];
        }

        delete [] data;
        data = next;

        current_size = next_size;
    }

    constexpr
    void push_back(const T& in)
    {
        if(current_size == idx)
        {
            upsize();
        }

        data[idx] = in;
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

        data[idx] = T();

        T& val = data[idx];

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

        return data[idx];
    }

    constexpr
    const T& operator[](std::size_t idx) const
    {
        if(!std::is_constant_evaluated())
        {
            assert(idx < current_size);
        }

        return data[idx];
    }

    constexpr
    size_t size() const
    {
        return idx;
    }

    constexpr
    auto begin()
    {
        return data;
    }

    constexpr
    auto end()
    {
        if(!std::is_constant_evaluated())
        {
            if(data == nullptr)
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
            assert(data);
        }

        return data[idx - 1];
    }

    constexpr
    const auto& back() const
    {
        if(!std::is_constant_evaluated())
        {
            assert(idx > 0);
            assert(data);
        }

        return data[idx - 1];
    }

    constexpr
    void clear()
    {
        idx = 0;
    }
};

#endif // HEAP_VECTOR_HPP_INCLUDED
