#ifndef STACK_VECTOR_HPP_INCLUDED
#define STACK_VECTOR_HPP_INCLUDED

template<typename T, int N>
struct stack_vector
{
    std::array<T, N> svec;
    size_t idx = 0;

    constexpr stack_vector() : svec{}, idx(0)
    {

    }

    constexpr
    void push_back(const T& in)
    {
        svec[idx] = in;
        idx++;
    }
};

#endif // STACK_VECTOR_HPP_INCLUDED
