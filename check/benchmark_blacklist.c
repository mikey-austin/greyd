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

#include <trie.h>
#include <blacklist.h>
#include <spamd_parser.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <zlib.h>
#include <time.h>

#define SEED 100
#define SAMPLES 50000

int
main(int argc, char *argv[])
{
    /* Load a sample blacklist. */
    Lexer_source_T ls;
    Lexer_T lexer;
    Spamd_parser_T parser;
    Blacklist_T bl;
    int ret;
    unsigned int seed = SEED;
    gzFile gzf;

    /* First arg is the seed. */
    if(argc > 1)
        seed = atoi(argv[1]);
    srand(seed);

    if((gzf = gzopen("data/traplist.gz", "r")) == NULL) {
        err(1, "gzdopen");
    }

    ls = Lexer_source_create_from_gz(gzf);
    lexer = Spamd_lexer_create(ls);
    bl = Blacklist_create("Test List", "You have been blacklisted");
    parser = Spamd_parser_create(lexer);
    ret = Spamd_parser_start(parser, bl, 0);

    printf("%d entries loaded\n", bl->count);

    /* Load into trie. */
    int i, j, r;
    struct Trie *t = NULL;
    struct Blacklist_entry *entry;
    unsigned char *bytes;
    struct IP_addr samples[SAMPLES];

    for(i = 0, j = 0; i < bl->count; i++) {
        entry = &bl->entries[i];
        entry->mask.addr32[0] = 0xFFFFFFFF;
        /* Mask out entries before storing in trie. */
        bytes = (unsigned char *) entry->address.addr8;
        if(bytes != NULL) {
            t = Trie_insert(t, bytes, 4);
        }

        if((i % 4) == (rand() % 4) && j < SAMPLES) {
            samples[j++].addr32[0] = entry->address.addr32[0];
        }
    }

    printf("trie loaded, %d samples selected\n\n", j);
    printf("--> test successful searches <--\n");

    clock_t begin, end;
    double spent;
    int errors;

    /* Time lookup of samples across blacklist. */
    begin = clock();
    for(i = 0, errors = 0; i < j; i++) {
        if(!Blacklist_match(bl, &samples[i], AF_INET))
            errors++;
    }
    end = clock();
    spent = (double) (end - begin) / CLOCKS_PER_SEC;
    printf("blacklist: %d searched, %d errors in %lf seconds\n", j, errors, spent);

    /* Time lookup across trie. */
    begin = clock();
    for(i = 0, errors = 0; i < j; i++) {
        bytes = (unsigned char *) samples[i].addr8;
        if(!Trie_contains(t, bytes, 4))
            errors++;
    }
    end = clock();
    spent = (double) (end - begin) / CLOCKS_PER_SEC;
    printf("trie: %d searched, %d errors in %lf seconds\n\n", j, errors, spent);

    /***********************************************************
     * For unsuccessful searches, mask out each sample address.
     */
    for(i = 0; i < j; i++) {
        samples[i].addr32[0] &= 0xFF000000;
    }

    printf("--> test unsuccessful searches <--\n");
    /* Time lookup of samples across blacklist. */
    begin = clock();
    for(i = 0, errors = 0; i < j; i++) {
        if(!Blacklist_match(bl, &samples[i], AF_INET))
            errors++;
    }
    end = clock();
    spent = (double) (end - begin) / CLOCKS_PER_SEC;
    printf("blacklist: %d searched, %d errors in %lf seconds\n", j, errors, spent);

    /* Time lookup across trie. */
    begin = clock();
    for(i = 0, errors = 0; i < j; i++) {
        bytes = (unsigned char *) samples[i].addr8;
        if(!Trie_contains(t, bytes, 4))
            errors++;
    }
    end = clock();
    spent = (double) (end - begin) / CLOCKS_PER_SEC;
    printf("trie: %d searched, %d errors in %lf seconds\n", j, errors, spent);

    /* Cleanup. */
    Spamd_parser_destroy(&parser);
    Blacklist_destroy(&bl);
    Trie_destroy(t);

    return 0;
}
