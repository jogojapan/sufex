# Sufex

## Overview

Sufex is an indexing and search system for linguistic purposes. It can
also be used as a C++ library of data structures relevant for
search. I am currently re-implementing it, and will add parts as they
get ready.

## How to build

    $> mkdir build
    $> cd build
    $> cmake ../src
    $> make

## Trigram generation and sorting

`sux/trigram.hpp` provides utilities for the generation and sorting of
trigrams as required by the DC algorithm for suffix array
construction.

### Trigram generation
Use the `sux::TrigramMaker<TGImpl,CharType,PosType>`. It takes three
template arguments:

 * The trigram implementation (`TGImpl::tuple`, `TGImpl::arraytuple`
   or `TGImpl::pointer`);
 * The character type (e.g. `char`)
 * The position type (e.g. `unsigned long`)

    #include <sux/trigram.hpp>
    
    using namespace std;
    using namespace sux;
    string input { "abcabeabxd" };
    
    /* Generating trigrams. */
    auto trigrams =
        TrigramMaker<TGImpl::tuple,string::value_type,size_t>::make_23trigrams(begin(input),end(input));
    
    /* Printing them. */
    for (const auto &trigram : trigrams)
      cout << triget1(trigram) << triget2(trigram) << triget3(trigram) << '\n';
    cout.flush();

The code above makes use of the functions `triget1`, `triget2` and
`triget3` to access the individual characters belonging to a trigram.

There is a **convenience function**, `sux::string_to_23trigrams`,
which can be applied to any instance of `std::basic_string<CharType>`.

### Trigram sorting

    /* Input. */
    string input { "abcabeabxd" };
    
    /* 2,3-Trigrams (convenience function). */
    auto trigrams = string_to_23trigrams(input);
    
    /* Trigram sorting using 2 parallel threads. */
    sort_23trigrams(trigrams,2);
