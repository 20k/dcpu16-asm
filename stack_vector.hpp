#ifndef STACK_VECTOR_HPP_INCLUDED
#define STACK_VECTOR_HPP_INCLUDED

#include <array>
#include <span>

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

    constexpr
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
    size_t size() const
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
    const auto& back() const
    {
        if(idx == 0)
            return svec[0];

        return svec[idx - 1];
    }

    constexpr
    auto data()
    {
        return &svec[0];
    }

    constexpr
    const auto data() const
    {
        return &svec[0];
    }

    constexpr
    void clear()
    {
        idx = 0;
    }

    constexpr
    std::span<T> as_span()
    {
        return std::span<T>{begin(), end()};
    }
};

#endif // STACK_VECTOR_HPP_INCLUDED
