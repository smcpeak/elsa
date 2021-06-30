// longlines3.c
// Long assignment statement.

typedef unsigned long long size_t;

typedef unsigned long long uint64_t;

typedef long long intptr_t;

struct av_oo_pointer_struct {
  uint64_t m_ordinal;
  intptr_t m_offset;
};

typedef struct av_oo_pointer_struct av_oo_pointer_t;

av_oo_pointer_t av_oo_ptr_malloc(size_t size);

void *av_oo_ptr_check(av_oo_pointer_t p, size_t size);

struct List {
  av_oo_pointer_t head;
  int x;
};

typedef struct List List;

av_oo_pointer_t av_oo_ptr_from_intptr(intptr_t n);

void g()
{
  av_oo_pointer_t __ptr_to_list = av_oo_ptr_malloc(16);
  (*((List *)av_oo_ptr_check(__ptr_to_list, 16))).head =
    av_oo_ptr_from_intptr(0);
  int y = (*((List *)av_oo_ptr_check(__ptr_to_list, 16))).x +
            av_oo_ptr_from_intptr(0).m_offset;
  y += (*((List *)av_oo_ptr_check(__ptr_to_list, 16))).x +
         av_oo_ptr_from_intptr(0).m_offset;
}

// EOF
