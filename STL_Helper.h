#ifndef STL_HELPER_H
#define STL_HELPER_H

#include <string>
#include <utility>
#include <vector>
#include <unordered_map>
#include <unordered_set>

/// TO STRING ///
namespace std
{
    template < typename S, typename T >
    std::string to_string(const std::pair< S, T > & x);

    template < typename K, typename V >
    std::string to_string(const std::unordered_map< K, V > & x);

    template < typename T>
    std::string to_string(const std::unordered_set< T > & x);

    template < typename T >
    std::string to_string(const std::vector< T > & x);
}

namespace std
{
    inline std::string to_string(char c)
    {
        return std::string(1, c);
    }
    
    inline std::string to_string(const std::string & x)
    {
        return x;
    }
    
    template < typename S, typename T >
    std::string to_string(const std::pair< S, T > & x)
    {
        return "(" + std::to_string(x.first) + "," +
            std::to_string(x.second) + ")";
    }

    template < typename K, typename V >
    std::string to_string(const std::unordered_map< K, V > & x)
    {
        std::string ret = "", delim = "";

        ret += "{";
        for (const std::pair< K, V > & pair : x)
        {
            ret += delim + std::to_string(pair.first) + ":" +
                std::to_string(pair.second);
            delim = ",";
        }
        ret += "}";

        return ret;
    }

    template < typename T>
    std::string to_string(const std::unordered_set< T > & x)
    {
        std::string ret = "", delim = "";

        ret += "{";
        for (const T & val : x)
        {
            ret += delim + std::to_string(val);
            delim = ",";
        }
        ret += "}";

        return ret;
    }

    template < typename T >
    std::string to_string(const std::vector< T > & x)
    {
        std::string ret = "", delim = "";

        ret += "[";
        for (const T & val : x)
        {
            ret += delim + std::to_string(val);
            delim = ",";
        }
        ret += "]";

        return ret;
    }
}


/// HASH ///
namespace std
{
    template < typename S, typename T >
    struct hash< std::pair< S, T > >
    {
        size_t operator()(const std::pair< S, T > & x) const
        {
            std::hash< std::string > hasher;
            return hasher(std::to_string(x));
        }
    };

    template < typename T >
    struct hash< std::unordered_set< T > >
    {
        size_t operator()(const std::unordered_set< T > & x) const
        {
            std::hash< std::string > hasher;
            return hasher(std::to_string(x));
        }
    };

    template < typename K, typename V>
    struct hash< std::unordered_map< K, V > >
    {
        size_t operator()(const std::unordered_map< K, V > & x) const
        {
            std::hash< std::string > hasher;
            return hasher(std::to_string(x));
        }
    };
}

// This is used as to not overwrite the std::merge function that exists,
// but the c++ version I am writing in does not have the std::merge
// function, so I made this one myself.
namespace helper
{
    // Given two sets, merge them into the first set. Does not
    // alter the second set.
    template< typename T >
    void merge(std::unordered_set< T > & s0,
               const std::unordered_set< T > & s1)
    {
        for (const T & val : s1)
            if (s0.find(val) == s0.end())
                s0.insert(val);

        return;
    }
}

#endif
