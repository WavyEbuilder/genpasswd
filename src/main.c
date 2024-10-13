#include <fcntl.h>
#include <sodium.h>
#include <stdio.h>
#include <string.h>
#include <tcl.h>
#include <unistd.h>

#define BUFFER_SIZE 8192

// Memory Management

struct allocation_node {
  struct allocation_node *next;
};

static struct allocation_node *alloc_head = NULL;

// Free all mallocs

static void free_allocations(void) {
  struct allocation_node *tmp;
  struct allocation_node *n;

  n = alloc_head;
  alloc_head = NULL;

  while (n != NULL) {
    tmp = n->next;
    free(n);
    n = tmp;
  }
}

// Track all mallocs
static void *malloc_wrapper(size_t size) {
  struct allocation_node *node;
  void *p;

  node = malloc(sizeof *node + size);
  if (node == NULL)
    abort();

  node->next = alloc_head;
  alloc_head = node;

  p = node + 1;
  return p;
}

// Dirty stack to expose zeroing errors
static void dirty(void) {
  unsigned char memory[8192];

  memset(memory, 'c', sizeof memory);
}

// Print HEX
static char *print_hex(const void *buf, const size_t len) {
  const unsigned char *b;
  char *p;

  b = buf;
  p = malloc_wrapper(len * 8 + 16);

  /* the library supplies a few utility functions like the one below */
  return sodium_bin2hex(p, len * 8 + 16, b, len);
}

int main(void) {
  dirty();
  unsigned char k[crypto_generichash_KEYBYTES_MAX]; // Key
  unsigned char h[crypto_generichash_BYTES_MAX];    // Hash output
  unsigned char m[BUFFER_SIZE];                     // Message
  size_t mlen = 0;

  if (sodium_init() < 0) {
    panic("FATAL ERROR: could NOT initialize cryptographic engine, "
          "aborting.\n"); // IT IS NOT SAFE TO RUN
  }
  printf("Cryptographic engine started successfully!\n");

  sodium_memzero(k, sizeof k);
  sodium_memzero(h, sizeof h);
  sodium_memzero(m, sizeof m);

  randombytes_buf(k, sizeof k);
  randombytes_buf(h, sizeof h);
  randombytes_buf(m, sizeof m);

  crypto_generichash(h, sizeof h, m, mlen, k, sizeof k);
  printf("Result: %s\n\n", print_hex(h, sizeof h));
  sodium_memzero(k, sizeof k);
  atexit(&free_allocations);
  return 0;
}
