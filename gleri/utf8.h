// This file is part of the GLERI project.
//
// Copyright (c) 2005 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.
//
// This file contains stream iterators that read and write UTF-8 encoded
// characters. The encoding is defined as follows:
//
// U-00000000 - U-0000007F: 0xxxxxxx
// U-00000080 - U-000007FF: 110xxxxx 10xxxxxx
// U-00000800 - U-0000FFFF: 1110xxxx 10xxxxxx 10xxxxxx
// U-00010000 - U-001FFFFF: 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
// U-00200000 - U-03FFFFFF: 111110xx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx
// U-04000000 - U-7FFFFFFF: 1111110x 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx
// U-80000000 - U-FFFFFFFF: 11111110 100000xx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx

#pragma once
#include "config.h"

//----------------------------------------------------------------------

/// Returns the number of bytes required to UTF-8 encode \p v.
inline CONST size_t Utf8Bytes (wchar_t v)
{
    if ((uint32_t) v < 128)
	return 1;
    size_t n;
    #if __x86__
        auto r = 0;
	asm("bsr\t%2, %k0\n\t"
	    "add\t$4, %0\n\t"
	    "div\t%3":"=a"(n),"+d"(r):"r"(v),"c"(5));
    #else
	static const uint32_t c_Bounds[7] = { 0x0000007F, 0x000007FF, 0x0000FFFF, 0x001FFFFF, 0x03FFFFFF, 0x7FFFFFFF, 0xFFFFFFFF };
	for (n = 0; c_Bounds[n++] < uint32_t(v););
    #endif
    return n;
}

/// Measures the size of a wchar_t array in UTF-8 encoding.
inline PURE size_t Utf8Bytes (const wchar_t* first, const wchar_t* last)
{
    size_t bc = 0;
    for (; first < last; ++first)
	bc += Utf8Bytes(*first);
    return bc;
}

/// Returns the number of bytes in a UTF-8 sequence that starts with \p c.
inline CONST size_t Utf8SequenceBytes (wchar_t c)	// a wchar_t to keep c in a full register
{
    // Count the leading bits. Header bits are 1 * nBytes followed by a 0.
    //	0 - single byte character. Take 7 bits (0xFF >> 1)
    //	1 - error, in the middle of the character. Take 6 bits (0xFF >> 2)
    //	    so you will keep reading invalid entries until you hit the next character.
    //	>2 - multibyte character. Take remaining bits, and get the next bytes.
    // All errors are ignored, since the user can not correct them.
    //
    wchar_t mask = 0x80;
    size_t nBytes = 0;
    for (; c & mask; ++nBytes)
	mask >>= 1;
    return nBytes ? nBytes : 1; // A sequence is always at least 1 byte.
}

//----------------------------------------------------------------------

/// \brief An iterator adaptor to character containers for reading UTF-8 encoded text.
///
/// For example, you can copy from ustl::string to ustl::vector<wchar_t> with
/// copy (utf8in (str.begin()), utf8in (str.end()), back_inserter(wvect));
/// There is no error handling; if the reading frame slips you'll get extra
/// characters, one for every misaligned byte. Although it is possible to skip
/// to the start of the next character, that would result in omitting the
/// misformatted character and the one after it, making it very difficult to
/// detect by the user. It is better to write some strange characters and let
/// the user know his file is corrupted. Another problem is overflow on bad
/// encodings (like a 0xFF on the end of a string). This is checked through
/// the end-of-string nul character, which will always be there as long as
/// you are using the string class.
///
template <typename Iterator, typename WChar = wchar_t>
class utf8in_iterator {
public:
    using value_type		= typename iterator_traits<Iterator>::value_type;
    using difference_type	= typename iterator_traits<Iterator>::difference_type;
    using pointer		= typename iterator_traits<Iterator>::pointer;
    using reference		= typename iterator_traits<Iterator>::reference;
public:
    explicit			utf8in_iterator (const Iterator& is)		: _i (is), _v (0) { Read(); }
				utf8in_iterator (const utf8in_iterator& i)	: _i (i._i), _v (i._v) {}
    inline const utf8in_iterator& operator= (const utf8in_iterator& i)		{ _i = i._i; _v = i._v; return *this; }
    inline Iterator		base (void) const				{ return _i - (Utf8Bytes(_v) - 1); }
    /// Reads and returns the next value.
    inline WChar		operator* (void) const				{ return _v; }
    inline utf8in_iterator&	operator++ (void)				{ ++_i; Read(); return *this; }
    inline utf8in_iterator	operator++ (int)				{ utf8in_iterator old (*this); operator++(); return old; }
    inline utf8in_iterator&	operator+= (difference_type n)			{ while (n--) operator++(); return *this; }
    inline utf8in_iterator	operator+ (difference_type n)			{ utf8in_iterator v (*this); return v += n; }
    inline bool			operator== (const utf8in_iterator& i) const	{ return _i == i._i; }
    inline bool			operator< (const utf8in_iterator& i) const	{ return _i < i._i; }
    inline bool			operator< (const Iterator& i) const		{ return _i < i; }
    /// Returns the distance in characters (as opposed to the distance in bytes).
    difference_type		operator- (const utf8in_iterator& i) const {
				    difference_type d = 0;
				    for (auto f (i._i); f < _i; ++d)
					f += Utf8SequenceBytes (*f);
				    return d;
				}
private:
    /// Steps to the next character and updates current returnable value.
    void			Read (void) {
				    const auto c = *_i;
				    auto nBytes = Utf8SequenceBytes (c);
				    _v = c & (0xFF >> nBytes);	// First byte contains bits after the header.
				    while (--nBytes && *++_i)	// Each subsequent byte has 6 bits.
					_v = (_v << 6) | (*_i & 0x3F);
				}
private:
    Iterator			_i;
    WChar			_v;
};

//----------------------------------------------------------------------

/// An iterator adaptor to character containers for writing UTF-8 encoded text.
template <typename Iterator, typename WChar = wchar_t>
class utf8out_iterator {
public:
    using value_type		= typename iterator_traits<Iterator>::value_type;
    using difference_type	= typename iterator_traits<Iterator>::difference_type;
    using pointer		= typename iterator_traits<Iterator>::pointer;
    using reference		= typename iterator_traits<Iterator>::reference;
public:
    explicit			utf8out_iterator (const Iterator& os)		: _i (os) {}
				utf8out_iterator (const utf8out_iterator& i)	: _i (i._i) {}
    inline const Iterator&	base (void) const				{ return _i; }
    inline utf8out_iterator&	operator* (void)				{ return *this; }
    inline utf8out_iterator&	operator++ (void)				{ return *this; }
    inline utf8out_iterator	operator++ (int)				{ return *this; }
    inline bool			operator== (const utf8out_iterator& i) const	{ return _i == i._i; }
    inline bool			operator< (const utf8out_iterator& i) const	{ return _i < i._i; }
    /// Writes \p v into the stream.
    utf8out_iterator&		operator= (WChar v) {
				    const auto nBytes = Utf8Bytes (v);
				    if (nBytes > 1) {
					// Write the bits 6 bits at a time, except for the first one,
					// which may be less than 6 bits.
					auto shift = nBytes * 6;
					*_i++ = ((v >> (shift -= 6)) & 0x3F) | (0xFF << (8 - nBytes));
					while (shift)
					    *_i++ = ((v >> (shift -= 6)) & 0x3F) | 0x80;
				    } else	// If only one byte, there is no header.
					*_i++ = v;
				    return *this;
				}
private:
    Iterator			_i;
};

//----------------------------------------------------------------------

/// Returns a UTF-8 adaptor writing to \p i. Useful in conjuction with back_insert_iterator.
template <typename Iterator>
inline utf8out_iterator<Iterator> utf8out (Iterator i)
    { return utf8out_iterator<Iterator> (i); }

/// Returns a UTF-8 adaptor reading from \p i.
template <typename Iterator>
inline utf8in_iterator<Iterator> utf8in (Iterator i)
    { return utf8in_iterator<Iterator> (i); }

//----------------------------------------------------------------------
