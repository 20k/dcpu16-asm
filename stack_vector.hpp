#ifndef STACK_VECTOR_HPP_INCLUDED
#define STACK_VECTOR_HPP_INCLUDED

#include <array>

template<typename T, int N>
struct stack_vector
{
    std::array<T, N> svec;
    size_t idx = 0;
    static constexpr int max_size = N;

    constexpr
    stack_vector() : svec{}, idx(0)
    {

    }

    constexpr
    void push_back(const T& in)
    {
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
        svec[idx] = T();

        T& val = svec[idx];

        idx++;

        return val;
    }

    constexpr
    T& operator[](std::size_t idx)
    {
        return svec[idx];
    }

    constexpr
    const T& operator[](std::size_t idx) const
    {
        return svec[idx];
    }

    constexpr
    size_t size()
    {
        return idx;
    }

    constexpr
    auto begin()
    {
        return svec.begin();
    }

    constexpr
    auto end()
    {
        return svec.begin() + idx;
    }

    constexpr
    auto& back()
    {
        if(idx == 0)
            return svec[0];

        return svec[idx - 1];
    }

    constexpr
    auto back() const
    {
        if(idx == 0)
            return svec[0];

        return svec[idx - 1];
    }
};

#endif // STACK_VECTOR_HPP_INCLUDED
