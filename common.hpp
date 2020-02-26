#ifndef COMMON_HPP_INCLUDED
#define COMMON_HPP_INCLUDED

#include <string_view>
#include <cctype>

template<typename T>
inline
constexpr std::string_view trim_start(std::string_view view, const T& matches)
{
    if(view.size() == 0)
        return view;

    int i=0;

    for(; i < (int)view.size() && matches(view[i]); i++)
    {

    }

    view.remove_prefix(i);

    return view;
}

template<typename T>
inline
constexpr std::string_view trim_end(std::string_view view, const T& matches)
{
    if(view.size() == 0)
        return view;

    int fin = (int)view.size();

    for(; (fin - 1) >= 0 && matches(view[fin - 1]); fin--)
    {

    }

    view.remove_suffix((int)view.size() - fin);

    return view;
}

inline
constexpr std::string_view consume_next(std::string_view& in)
{
    if(in.size() == 0)
        return in;

    auto trimmed = trim_start(in, [](char c) {return c == ' ' || c == '\n' || c == ','; });

    if(trimmed.size() == 0)
    {
        in = trimmed;
        return trimmed;
    }

    size_t next = std::min(trimmed.find_first_of(",\n "), trimmed.size());

    auto first = trimmed;
    auto second = trimmed;

    first.remove_suffix(trimmed.size() - next);
    second.remove_prefix(next);

    in = second;

    return trim_end(first, [](char c) {return c == ' ' || c == '\n' || c == ','; });
}

inline
constexpr bool is_label(std::string_view in)
{
    return in.starts_with(':');
}

inline
constexpr bool is_hex_digit(char in)
{
    return
    (in >= '0' && in <= '9') ||
    in == 'a' || in == 'A' ||
    in == 'b' || in == 'B' ||
    in == 'c' || in == 'C' ||
    in == 'd' || in == 'D' ||
    in == 'e' || in == 'E' ||
    in == 'f' || in == 'F';
}

inline
constexpr bool is_digit(char in)
{
    return in >= '0' && in <= '9';
}

inline
constexpr bool is_constant(std::string_view in)
{
    if(in.size() == 0)
        return false;

    int start_idx = 0;

    if(in.starts_with('-'))
    {
        start_idx = 1;
    }

    in.remove_prefix(start_idx);

    if(in.starts_with("0x"))
    {
        for(int i=2; i < (int)in.size(); i++)
        {
            if(!is_hex_digit(in[i]))
                return false;
        }

        return true;
    }

    for(int i=0; i < (int)in.size(); i++)
    {
        if(!is_digit(in[i]))
            return false;
    }

    return true;
}

#endif // COMMON_HPP_INCLUDED
