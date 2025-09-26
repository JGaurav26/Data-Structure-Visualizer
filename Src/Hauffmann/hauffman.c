/* huffman.c
 *
 * Simple Huffman compression & decompression in C.
 * Usage:
 *   Compile: gcc -std=c11 -O2 -o huffman huffman.c
 *   Encode:  ./huffman encode  input.txt  output.huf
 *   Decode:  ./huffman decode  output.huf restored.txt
 *
 * Notes:
 *  - This implementation supports full 0..255 byte values.
 *  - The compressed file format:
 *      [uint16_t unique_count]
 *      For each unique symbol:
 *        [uint8_t symbol][uint32_t frequency]
 *      Then: compressed bitstream (remaining bytes) — no explicit length; decoder reads until EOF.
 *
 * This is designed for teaching: min-heap for priority queue, binary tree for Huffman tree,
 * recursive code generation, and bit-buffered I/O.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define SYMBOLS 256

/* Huffman tree node */
typedef struct Node {
    unsigned char symbol;
    uint32_t freq;
    struct Node *left, *right;
} Node;

/* Min-heap structure */
typedef struct MinHeap {
    int size;
    int capacity;
    Node **array; // array of pointers to nodes
} MinHeap;

/* Bit writer and reader */
typedef struct BitWriter {
    FILE *f;
    unsigned char buffer;
    int bit_count; /* how many bits currently in buffer (0..7) */
} BitWriter;

typedef struct BitReader {
    FILE *f;
    unsigned char buffer;
    int bit_count; /* bits left in buffer (0..7) */
    int eof_reached;
} BitReader;

/* Prototypes */
Node *create_node(unsigned char symbol, uint32_t freq);
MinHeap *create_minheap(int capacity);
void minheap_insert(MinHeap *h, Node *node);
Node *minheap_extract(MinHeap *h);
void minheapify(MinHeap *h, int idx);
Node *build_huffman_tree(uint32_t freq_table[]);
void build_codes(Node *root, char *code, int depth, char *codes[]);
void free_tree(Node *root);

/* File header helpers */
int write_header(FILE *out, uint32_t freq_table[]);
int read_header(FILE *in, uint32_t freq_table[]);

/* Bit I/O helpers */
BitWriter *bitwriter_create(FILE *f);
void bitwriter_write_bit(BitWriter *bw, int bit);
void bitwriter_write_bits(BitWriter *bw, const char *bits);
void bitwriter_flush(BitWriter *bw);
void bitwriter_free(BitWriter *bw);

BitReader *bitreader_create(FILE *f);
int bitreader_read_bit(BitReader *br);
void bitreader_free(BitReader *br);

/* High-level compress/decompress */
int huffman_encode(const char *infile, const char *outfile);
int huffman_decode(const char *infile, const char *outfile);

/* Utility */
void print_usage(const char *prog);

/* === Implementation === */

Node *create_node(unsigned char symbol, uint32_t freq) {
    Node *n = (Node *)malloc(sizeof(Node));
    if (!n) { perror("malloc"); exit(EXIT_FAILURE); }
    n->symbol = symbol;
    n->freq = freq;
    n->left = n->right = NULL;
    return n;
}

MinHeap *create_minheap(int capacity) {
    MinHeap *h = (MinHeap *)malloc(sizeof(MinHeap));
    if (!h) { perror("malloc"); exit(EXIT_FAILURE); }
    h->size = 0;
    h->capacity = capacity;
    h->array = (Node **)malloc(capacity * sizeof(Node *));
    if (!h->array) { perror("malloc"); exit(EXIT_FAILURE); }
    return h;
}

void swap_nodes(Node **a, Node **b) {
    Node *t = *a;
    *a = *b;
    *b = t;
}

void minheap_insert(MinHeap *h, Node *node) {
    if (h->size == h->capacity) {
        h->capacity *= 2;
        h->array = (Node **)realloc(h->array, h->capacity * sizeof(Node *));
        if (!h->array) { perror("realloc"); exit(EXIT_FAILURE); }
    }
    int i = h->size++;
    h->array[i] = node;
    /* sift up */
    while (i && h->array[(i - 1) / 2]->freq > h->array[i]->freq) {
        swap_nodes(&h->array[i], &h->array[(i - 1) / 2]);
        i = (i - 1) / 2;
    }
}

void minheapify(MinHeap *h, int idx) {
    int smallest = idx;
    int l = 2 * idx + 1;
    int r = 2 * idx + 2;
    if (l < h->size && h->array[l]->freq < h->array[smallest]->freq) smallest = l;
    if (r < h->size && h->array[r]->freq < h->array[smallest]->freq) smallest = r;
    if (smallest != idx) {
        swap_nodes(&h->array[smallest], &h->array[idx]);
        minheapify(h, smallest);
    }
}

Node *minheap_extract(MinHeap *h) {
    if (h->size == 0) return NULL;
    Node *root = h->array[0];
    h->array[0] = h->array[--h->size];
    minheapify(h, 0);
    return root;
}

Node *build_huffman_tree(uint32_t freq_table[]) {
    MinHeap *h = create_minheap(256);
    for (int i = 0; i < SYMBOLS; ++i) {
        if (freq_table[i] > 0) {
            minheap_insert(h, create_node((unsigned char)i, freq_table[i]));
        }
    }

    /* Special case: only one unique symbol -> create a tree with two nodes so codes work */
    if (h->size == 1) {
        Node *only = minheap_extract(h);
        Node *parent = create_node(0, only->freq);
        parent->left = only;
        parent->right = create_node(0, 0);
        minheap_insert(h, parent);
    }

    while (h->size > 1) {
        Node *left = minheap_extract(h);
        Node *right = minheap_extract(h);
        Node *parent = create_node(0, left->freq + right->freq);
        parent->left = left;
        parent->right = right;
        minheap_insert(h, parent);
    }

    Node *root = minheap_extract(h);
    free(h->array);
    free(h);
    return root;
}

void build_codes(Node *root, char *code, int depth, char *codes[]) {
    if (!root) return;
    if (!root->left && !root->right) {
        code[depth] = '\0';
        codes[root->symbol] = strdup(code);
        return;
    }
    if (root->left) {
        code[depth] = '0';
        build_codes(root->left, code, depth + 1, codes);
    }
    if (root->right) {
        code[depth] = '1';
        build_codes(root->right, code, depth + 1, codes);
    }
}

void free_tree(Node *root) {
    if (!root) return;
    free_tree(root->left);
    free_tree(root->right);
    free(root);
}

/* Write header: number of unique bytes + list of (symbol, freq) */
int write_header(FILE *out, uint32_t freq_table[]) {
    uint16_t unique = 0;
    for (int i = 0; i < SYMBOLS; ++i) if (freq_table[i]) ++unique;
    if (fwrite(&unique, sizeof(unique), 1, out) != 1) return 0;
    for (int i = 0; i < SYMBOLS; ++i) {
        if (freq_table[i]) {
            uint8_t sym = (uint8_t)i;
            uint32_t f = freq_table[i];
            if (fwrite(&sym, sizeof(sym), 1, out) != 1) return 0;
            if (fwrite(&f, sizeof(f), 1, out) != 1) return 0;
        }
    }
    return 1;
}

/* Read header, fill freq_table (caller must zero table beforehand) */
int read_header(FILE *in, uint32_t freq_table[]) {
    uint16_t unique;
    if (fread(&unique, sizeof(unique), 1, in) != 1) return 0;
    for (int i = 0; i < unique; ++i) {
        uint8_t sym;
        uint32_t f;
        if (fread(&sym, sizeof(sym), 1, in) != 1) return 0;
        if (fread(&f, sizeof(f), 1, in) != 1) return 0;
        freq_table[sym] = f;
    }
    return 1;
}

/* Bit writer implementation */
BitWriter *bitwriter_create(FILE *f) {
    BitWriter *bw = (BitWriter *)malloc(sizeof(BitWriter));
    bw->f = f;
    bw->buffer = 0;
    bw->bit_count = 0;
    return bw;
}

void bitwriter_write_bit(BitWriter *bw, int bit) {
    if (bit) bw->buffer |= (1 << (7 - bw->bit_count));
    bw->bit_count++;
    if (bw->bit_count == 8) {
        fputc(bw->buffer, bw->f);
        bw->buffer = 0;
        bw->bit_count = 0;
    }
}

void bitwriter_write_bits(BitWriter *bw, const char *bits) {
    for (const char *p = bits; *p; ++p) {
        bitwriter_write_bit(bw, (*p == '1'));
    }
}

void bitwriter_flush(BitWriter *bw) {
    if (bw->bit_count > 0) {
        fputc(bw->buffer, bw->f);
        bw->buffer = 0;
        bw->bit_count = 0;
    }
}

void bitwriter_free(BitWriter *bw) {
    free(bw);
}

/* Bit reader implementation */
BitReader *bitreader_create(FILE *f) {
    BitReader *br = (BitReader *)malloc(sizeof(BitReader));
    br->f = f;
    br->buffer = 0;
    br->bit_count = 0;
    br->eof_reached = 0;
    return br;
}

/* Returns -1 on EOF, else 0 or 1 */
int bitreader_read_bit(BitReader *br) {
    if (br->bit_count == 0) {
        int c = fgetc(br->f);
        if (c == EOF) {
            br->eof_reached = 1;
            return -1;
        }
        br->buffer = (unsigned char)c;
        br->bit_count = 8;
    }
    int bit = (br->buffer & (1 << (7 - (8 - br->bit_count)))) ? 1 : 0;
    br->bit_count--;
    return bit;
}

void bitreader_free(BitReader *br) {
    free(br);
}

/* Encode an input file to output file */
int huffman_encode(const char *infile, const char *outfile) {
    FILE *fin = fopen(infile, "rb");
    if (!fin) { perror("fopen input"); return 0; }

    /* Build frequency table */
    uint32_t freq_table[SYMBOLS] = {0};
    int c;
    uint64_t total_bytes = 0;
    while ((c = fgetc(fin)) != EOF) {
        freq_table[(unsigned char)c]++;
        total_bytes++;
    }
    if (total_bytes == 0) {
        fprintf(stderr, "Input file is empty.\n");
        fclose(fin);
        return 0;
    }

    /* Rewind to read file again while writing compressed content */
    rewind(fin);

    /* Build Huffman tree */
    Node *root = build_huffman_tree(freq_table);

    /* Generate codes */
    char *codes[SYMBOLS] = {0};
    char codebuf[SYMBOLS];
    build_codes(root, codebuf, 0, codes);

    /* Open output and write header */
    FILE *fout = fopen(outfile, "wb");
    if (!fout) { perror("fopen output"); fclose(fin); free_tree(root); return 0; }
    if (!write_header(fout, freq_table)) { fprintf(stderr, "Error writing header\n"); fclose(fin); fclose(fout); free_tree(root); return 0; }

    /* Bit writer */
    BitWriter *bw = bitwriter_create(fout);

    /* Read input and write codes as bits */
    while ((c = fgetc(fin)) != EOF) {
        unsigned char sym = (unsigned char)c;
        if (!codes[sym]) { fprintf(stderr, "Missing code for symbol %u\n", sym); continue; }
        bitwriter_write_bits(bw, codes[sym]);
    }

    /* Flush remaining bits */
    bitwriter_flush(bw);

    /* Cleanup */
    for (int i = 0; i < SYMBOLS; ++i) if (codes[i]) free(codes[i]);
    bitwriter_free(bw);
    fclose(fin);
    fclose(fout);
    free_tree(root);

    return 1;
}

/* Decode compressed file */
int huffman_decode(const char *infile, const char *outfile) {
    FILE *fin = fopen(infile, "rb");
    if (!fin) { perror("fopen input"); return 0; }

    uint32_t freq_table[SYMBOLS] = {0};
    if (!read_header(fin, freq_table)) { fprintf(stderr, "Error reading header or invalid file\n"); fclose(fin); return 0; }

    Node *root = build_huffman_tree(freq_table);
    if (!root) { fprintf(stderr, "Failed to rebuild Huffman tree\n"); fclose(fin); return 0; }

    FILE *fout = fopen(outfile, "wb");
    if (!fout) { perror("fopen output"); fclose(fin); free_tree(root); return 0; }

    BitReader *br = bitreader_create(fin);
    Node *cur = root;
    int bit;
    while ((bit = bitreader_read_bit(br)) != -1) {
        if (bit == 0) cur = cur->left;
        else cur = cur->right;

        if (!cur->left && !cur->right) {
            /* Leaf node: output symbol */
            fputc(cur->symbol, fout);
            cur = root;
        }
    }

    /* Close and cleanup */
    bitreader_free(br);
    fclose(fin);
    fclose(fout);
    free_tree(root);
    return 1;
}

void print_usage(const char *prog) {
    fprintf(stderr, "Usage:\n  %s encode  input_file  output_file\n  %s decode  input_file  output_file\n", prog, prog);
}

/* main */
int main(int argc, char **argv) {
    if (argc != 4) {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }
    if (strcmp(argv[1], "encode") == 0) {
        if (huffman_encode(argv[2], argv[3])) {
            printf("Encoding completed: %s -> %s\n", argv[2], argv[3]);
            return EXIT_SUCCESS;
        } else {
            fprintf(stderr, "Encoding failed.\n");
            return EXIT_FAILURE;
        }
    } else if (strcmp(argv[1], "decode") == 0) {
        if (huffman_decode(argv[2], argv[3])) {
            printf("Decoding completed: %s -> %s\n", argv[2], argv[3]);
            return EXIT_SUCCESS;
        } else {
            fprintf(stderr, "Decoding failed.\n");
            return EXIT_FAILURE;
        }
    } else {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }
}
