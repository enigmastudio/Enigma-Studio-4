/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *
 *   This file is part of
 *       ______        _                             __ __
 *      / ____/____   (_)____ _ ____ ___   ____ _   / // /
 *     / __/  / __ \ / // __ `// __ `__ \ / __ `/  / // /_
 *    / /___ / / / // // /_/ // / / / / // /_/ /  /__  __/
 *   /_____//_/ /_//_/ \__, //_/ /_/ /_/ \__,_/     /_/.   
 *                    /____/                              
 *
 *   Copyright © 2003-2012 Brain Control, all rights reserved.
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef STRING_HPP
#define STRING_HPP

// helper template structs for compile time string hashes
// (taken from "Quasi compile-time string hashing" article)

template<eU32 N, eU32 I> struct eFnvHash
{
    eINLINE static eU32 hash(const eChar (&str)[N])
    {
        return (eFnvHash<N, I-1>::hash(str)^str[I-1])*16777619;
    }
};

template<eU32 N> struct eFnvHash<N, 1>
{
    eINLINE static eU32 hash(const eChar (&str)[N])
    {
        return (2166136261^str[0])*16777619;
    }
};

// use this function to calculate a
// string hash at compile time
template<eU32 N> eU32 eHashStr(const eChar (&str)[N])
{
    return eFnvHash<N, N>::hash(str);
}

// dynamic string class
class eString
{
public:
    eString();
    eString(eChar chr);
    eString(const eChar *str);
    eString(const eChar *str, eU32 length);
    eString(const eString &str);

    eU32            length() const;
    eBool           equals(const eString &str, eU32 count) const;

    void            padLeft(eU32 totalLen, eChar chr);
    void            makeUpper();
    eBool           split(eChar token, eString &left, eString &right) const;
    eString         subStr(eU32 startIndex, eU32 endIndex) const;
    eString         simplified() const;
    void            remove(eU32 startIndex, eU32 endIndex);
    void            removeAt(eU32 index);

    eString         operator + (const eString &str) const;
    eString &       operator += (eChar c);
    eString &       operator += (const eString &str);
    eString &       operator = (const eChar *str);

    const eChar &   at(eU32 index) const;
    eChar &         at(eU32 index);
    const eChar &   operator [] (eInt index) const;
    eChar &         operator [] (eInt index);

    eBool           operator == (const eString &str) const;
    eBool           operator == (const eChar *str) const;
    eBool           operator != (const eString &str) const;
    eBool           operator != (const eChar *str) const;

    operator const eChar * () const;

private:
    eArray<eChar>   m_data;
};

#endif // STRING_HPP