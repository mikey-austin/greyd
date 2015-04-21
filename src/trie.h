/*
 * Copyright (c) 2015 Mikey Austin <mikey@greyd.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/**
 * @file   trie.h
 * @brief  Defines the interface for a radix-trie (base 2).
 * @author Mikey Austin
 * @date   2015
 */

#ifndef TRIE_DEFINED
#define TRIE_DEFINED

#define TRIE_RADIX 2

struct Trie {
    unsigned char *key;
    int klen;
    int branch;
    struct Trie *kids[TRIE_RADIX];
    int (*cmp)(const void *, int, const void *, int);
};

extern struct Trie *Trie_create(const unsigned char *key, int klen,
             int (*cmp)(const void *, int, const void *, int));

extern struct Trie *Trie_insert(struct Trie *trie, const unsigned char *key, int klen);

extern int Trie_contains(struct Trie *trie, const unsigned char *key, int klen);

extern void Trie_destroy(struct Trie *trie);

#endif
