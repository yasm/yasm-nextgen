#ifndef YASM_HAMT_H
#define YASM_HAMT_H
///
/// @file libyasm/hamt.h
/// @brief Hash Array Mapped Trie (HAMT) implementation.
///
/// @license
///  Copyright (C) 2001-2007  Peter Johnson
///
/// Based on the paper "Ideal Hash Tries" by Phil Bagwell [2000].
/// One algorithmic change from that described in the paper: we use the LSB's
/// of the key to index the root table and move upward in the key rather than
/// use the MSBs as described in the paper.  The LSBs have more entropy.
///
/// Redistribution and use in source and binary forms, with or without
/// modification, are permitted provided that the following conditions
/// are met:
/// 1. Redistributions of source code must retain the above copyright
///    notice, this list of conditions and the following disclaimer.
/// 2. Redistributions in binary form must reproduce the above copyright
///    notice, this list of conditions and the following disclaimer in the
///    documentation and/or other materials provided with the distribution.
///
/// THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND OTHER CONTRIBUTORS ``AS IS''
/// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
/// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
/// ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR OTHER CONTRIBUTORS BE
/// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
/// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
/// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
/// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
/// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
/// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
/// POSSIBILITY OF SUCH DAMAGE.
/// @endlicense
///
#include <cctype>
#include <cstdlib>

#include <boost/noncopyable.hpp>
#include <boost/pool/pool.hpp>
#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/scoped_array.hpp>

#include "bitcount.h"


namespace yasm {

/// Hash array mapped trie data structure.
/// Template parameters:
/// - Key: class that keys the data (e.g. std::string)
/// - T: class that contains the data
/// - GetKey: functor that gets a key from the data;
///   definition should be: "Key GetKey(const T*)" or similar.
template <typename Key, typename T, typename GetKey>
class hamt : private boost::noncopyable {
public:
    /// Constructor.
    /// @param  nocase      True if HAMT should be case-insensitive
    hamt(bool nocase);

    /// Destructor.
    ~hamt();

    /// Search for the data associated with a key in the HAMT.
    /// @param key          Key
    /// @return NULL if key/data not present, otherwise data.
    T* find(const Key& key);

    /// Insert keyed data into HAMT, without replacement.
    /// @param data         Data to insert
    /// @return If key was already present, data from HAMT with that key.
    ///         If key was not present, NULL.
    T* insert(T* data) { return insrep(data, false); }

    /// Insert keyed data into HAMT, with replacement.
    /// @param data         Data to insert
    /// @return If key was already present, old data from HAMT with that key.
    ///         If key was not present, NULL.
    T* replace(T* data) { return insrep(data, true); }

private:
    /// A node.
    /// Nodes come in two forms:
    /// (a) a value-containing node that contains:
    ///     - bitmap_key = hash key of value
    ///     - value = pointer to value
    /// (b) a node tree that contains:
    ///     - bitmap_key = bitmap (1=present), indexed by 5 bits of hash
    ///     - value = 0
    ///     - followed in memory by between 1 and 32 Node*'s
    struct Node {
        unsigned long bitmap_key;   ///< 32 bits, bitmap or hash key

        /// Value pointer; if NULL an array of nodes immediately follow this
        /// node structure
        T* value;

        Node* & sub_trie(unsigned long i)
        { return (reinterpret_cast<Node**>(this+1))[i]; }
    };

    boost::scoped_array<Node*> m_root;
    bool m_nocase;

    // Pools are used to manage all node memory except for the root (above).
    // There are individual pools for each size, from 0-32 Node* members.
    // (0=value node, 32=completely full bitmapped array of nodes).
    boost::ptr_vector<boost::pool<> > m_pools;

    GetKey get_key; // functor instance

    /// Insert or replace keyed data into HAMT.
    /// @param data     Data to insert/replace
    /// @param replace  True if should replace
    /// @return Old data from HAMT; if no old data, NULL.
    T* insrep(T* data, bool replace);

    static unsigned long hash_key(const Key& key);
    static unsigned long rehash_key(const Key& key, int Level);
    static unsigned long hash_key_nocase(const Key& key);
    static unsigned long rehash_key_nocase(const Key& key, int Level);
};

template <typename Key, typename T, typename GetKey>
unsigned long
hamt<Key,T,GetKey>::hash_key(const Key& key)
{
    unsigned long a=31415, b=27183, vHash=0;
    for (typename Key::const_iterator i=key.begin(), end=key.end();
         i != end; ++i, a*=b)
        vHash = a*vHash + *i;
    return vHash;
}

template <typename Key, typename T, typename GetKey>
unsigned long
hamt<Key,T,GetKey>::rehash_key(const Key& key, int Level)
{
    unsigned long a=31415, b=27183, vHash=0;
    for (typename Key::const_iterator i=key.begin(), end=key.end();
         i != end; ++i, a*=b)
        vHash = a*vHash*(unsigned long)Level + *i;
    return vHash;
}

template <typename Key, typename T, typename GetKey>
unsigned long
hamt<Key,T,GetKey>::hash_key_nocase(const Key& key)
{
    unsigned long a=31415, b=27183, vHash=0;
    for (typename Key::const_iterator i=key.begin(), end=key.end();
         i != end; ++i, a*=b)
        vHash = a*vHash + std::tolower(*i);
    return vHash;
}

template <typename Key, typename T, typename GetKey>
unsigned long
hamt<Key,T,GetKey>::rehash_key_nocase(const Key& key, int Level)
{
    unsigned long a=31415, b=27183, vHash=0;
    for (typename Key::const_iterator i=key.begin(), end=key.end();
         i != end; ++i, a*=b)
        vHash = a*vHash*(unsigned long)Level + std::tolower(*i);
    return vHash;
}

template <typename Key, typename T, typename GetKey>
hamt<Key,T,GetKey>::hamt(bool nocase)
    : m_root(new Node* [32]),
      m_nocase(nocase)
{
    m_pools.reserve(33);
    for (int i=0; i<33; i++)
        m_pools.push_back(new boost::pool<>(sizeof(Node) + i*sizeof(Node*)));
    for (int i=0; i<32; i++)
        m_root[i] = 0;
}

template <typename Key, typename T, typename GetKey>
hamt<Key,T,GetKey>::~hamt()
{
}

template <typename Key, typename T, typename GetKey>
T*
hamt<Key,T,GetKey>::insrep(T* data, bool replace)
{
    unsigned long key = hash_key(get_key(data));
    unsigned long keypart = key & 0x1F;
    Node** pnode = &m_root[keypart];
    Node* node = *pnode;

    if (node == 0) {
        Node* node = static_cast<Node*>(m_pools[0].malloc());
        *pnode = node;
        node->bitmap_key = key;
        node->value = data;
        return 0;
    }

    for (int keypartbits=0, level=0;;) {
        if (node->value != 0) {
            if (node->bitmap_key == key &&
                ((!m_nocase && get_key(data) == get_key(node->value)) ||
                 (m_nocase && false/* TODO */))) {
                T* oldvalue = node->value;
                if (replace)
                    node->value = data;
                return oldvalue;
            } else {
                unsigned long key2 = node->bitmap_key;
                // build tree downward until keys differ
                for (;;) {
                    // replace node with subtrie
                    keypartbits += 5;
                    if (keypartbits > 30) {
                        // Exceeded 32 bits: rehash
                        key = rehash_key(get_key(data), level);
                        key2 = rehash_key(get_key(node->value), level);
                        keypartbits = 0;
                    }
                    keypart = (key >> keypartbits) & 0x1F;
                    unsigned long keypart2 = (key2 >> keypartbits) & 0x1F;

                    if (keypart == keypart2) {
                        // Still equal, build one-node subtrie and continue
                        // downward.
                        Node* newnode =
                            static_cast<Node*>(m_pools[1].malloc());
                        newnode->bitmap_key = 1<<keypart;
                        newnode->value = 0;  // subtrie indication
                        newnode->sub_trie(0) = node;
                        *pnode = newnode;

                        pnode = &newnode->sub_trie(0);
                        node = *pnode;
                        level++;
                    } else {
                        // partitioned

                        // create value node
                        Node* entry = static_cast<Node*>(m_pools[0].malloc());
                        entry->bitmap_key = key;
                        entry->value = data;

                        // update other node's bitmap in case we rehashed
                        node->bitmap_key = key2;

                        // allocate two-node subtrie
                        Node* newnode =
                            static_cast<Node*>(m_pools[2].malloc());

                        // Set bits in bitmap corresponding to keys
                        newnode->bitmap_key =
                            (1UL<<keypart) | (1UL<<keypart2);
                        newnode->value = 0;  // subtrie indication

                        // Copy nodes into subtrie based on order
                        if (keypart2 < keypart) {
                            newnode->sub_trie(0) = node;
                            newnode->sub_trie(1) = entry;
                        } else {
                            newnode->sub_trie(0) = entry;
                            newnode->sub_trie(1) = node;
                        }

                        *pnode = newnode;
                        return 0;
                    }
                }
            }
        }

        // Subtrie: look up in bitmap
        keypartbits += 5;
        if (keypartbits > 30) {
            // Exceeded 32 bits of current key: rehash
            key = rehash_key(get_key(data), level);
            keypartbits = 0;
        }
        keypart = (key >> keypartbits) & 0x1F;
        if (!(node->bitmap_key & (1<<keypart))) {
            // bit is 0 in bitmap -> add node to table

            // create value node
            Node* entry = static_cast<Node*>(m_pools[0].malloc());
            entry->bitmap_key = key;
            entry->value = data;

            // set bit to 1
            node->bitmap_key |= (1<<keypart);

            // Count total number of bits in bitmap to determine new size
            unsigned long size = bit_count(node->bitmap_key);
            size &= 0x1F;
            if (size == 0)
                size = 32;

            // Allocate new subtrie
            Node* newnode = static_cast<Node*>(m_pools[size].malloc());

            newnode->bitmap_key = node->bitmap_key;
            newnode->value = 0;     // subtrie indication

            // Count bits below to find where to insert new node at
            unsigned long map =
                bit_count(node->bitmap_key & ~((~0UL)<<keypart));
            map &= 0x1F;        // Clamp to <32

            // Copy existing nodes leaving gap for new node
            std::memcpy(&newnode->sub_trie(0), &node->sub_trie(0),
                        map*sizeof(Node*));
            std::memcpy(&newnode->sub_trie(map+1), &node->sub_trie(map),
                        (size-map-1)*sizeof(Node*));

            // Set up new node
            newnode->sub_trie(map) = entry;

            // Delete old subtrie
            m_pools[size-1].free(node);

            *pnode = newnode;
            return 0;
        }

        // Count bits below
        unsigned long map = bit_count(node->bitmap_key & ~((~0UL)<<keypart));
        map &= 0x1F;    // Clamp to <32

        // Go down a level
        level++;
        pnode = &node->sub_trie(map);
        node = *pnode;
    }
}

template <typename Key, typename T, typename GetKey>
T*
hamt<Key,T,GetKey>::find(const Key& str)
{
    unsigned long key = m_nocase ? hash_key_nocase(str) : hash_key(str);
    unsigned long keypart = key & 0x1F;
    Node* node = m_root[keypart];

    if (node == 0)
        return 0;

    for (int keypartbits=0, level=0;;) {
        if (node->value != 0) {
            if (node->bitmap_key == key &&
                ((!m_nocase && str == get_key(node->value)) ||
                 (m_nocase && false/* TODO */)))
                return node->value;
            else
                return 0;
        }

        // Subtree: look up in bitmap
        keypartbits += 5;
        if (keypartbits > 30) {
            // Exceeded 32 bits of current key: rehash
            key = rehash_key(str, level);
            keypartbits = 0;
        }
        keypart = (key >> keypartbits) & 0x1F;
        if (!(node->bitmap_key & (1<<keypart)))
            return 0;       // bit is 0 in bitmap -> no match

        // Count bits below
        unsigned long map = bit_count(node->bitmap_key & ~((~0UL)<<keypart));
        map &= 0x1F;        // Clamp to <32

        // Go down a level
        level++;
        node = node->sub_trie(map);
    }
}

} // namespace yasm

#endif
