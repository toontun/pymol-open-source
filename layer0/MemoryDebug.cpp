/* 
A* -------------------------------------------------------------------
B* This file contains source code for the PyMOL computer program
C* Copyright (c) Schrodinger, LLC. 
D* -------------------------------------------------------------------
E* It is unlawful to modify or remove this copyright notice.
F* -------------------------------------------------------------------
G* Please see the accompanying LICENSE file for further information. 
H* -------------------------------------------------------------------
I* Additional authors of this source file include:
-* 
-* 
-*
Z* -------------------------------------------------------------------
*/


/* This file can be compiled under C as a .c file, or under C++ as a .cc file*/

#include"os_predef.h"
#include"ov_port.h"

#include"MemoryDebug.h"
#include"MemoryCache.h"

#define GDB_ENTRY

void *MemoryReallocForSureSafe(void *ptr, unsigned int new_size, unsigned int old_size)
{
  if(new_size < old_size) {
    float *tmp = (float*) mmalloc(new_size);
    if(tmp && new_size && old_size) {
      memcpy(tmp, ptr, new_size);
    }
    FreeP(ptr);
    return tmp;
  } else {
    return mrealloc(ptr, new_size);
  }
}

void *MemoryReallocForSure(void *ptr, unsigned int new_size)
{                               /* unsafe -- replace with above */
  float *tmp = (float*) mmalloc(new_size);
  if(tmp)
    memcpy(tmp, ptr, new_size);
  FreeP(ptr);
  return tmp;
}

static void DieOutOfMemory(void)
{
  printf
    ("****************************************************************************\n");
  printf
    ("*** EEK!  PyMOL just ran out of memory and crashed.  To get around this, ***\n");
  printf
    ("*** you may need to reduce the quality, size, or complexity of the scene ***\n");
  printf
    ("*** that you are viewing or rendering.    Sorry for the inconvenience... ***\n");
  printf
    ("****************************************************************************\n");
#ifdef GDB_ENTRY
  abort();
#endif

  exit(EXIT_FAILURE);
}

void MemoryZero(char *p, char *q)
{
  if(q - p)
    memset(p, 0, q - p);
}

void *VLAExpand(void *ptr, ov_size rec)
{
  VLARec *vla;
  char *start, *stop;
  unsigned int soffset = 0;
  vla = &(((VLARec *) ptr)[-1]);
  if(rec >= vla->size) {
    if(vla->auto_zero)
      soffset = sizeof(VLARec) + (vla->unit_size * vla->size);
    vla->size = ((unsigned int) (rec * vla->grow_factor)) + 1;
#if 0
    if(vla->size <= rec)
      vla->size = rec + 1;
#endif
    {
      VLARec *old_vla = vla;
      vla = (VLARec *) mrealloc(vla, (vla->unit_size * vla->size) + sizeof(VLARec));
      while(!vla) {             /* back off on the request size until it actually fits */
        vla = old_vla;
        vla->grow_factor = (vla->grow_factor - 1.0F) / 2.0F + 1.0F;
        vla->size = ((unsigned int) (rec * vla->grow_factor)) + 1;
        vla = (VLARec *) mrealloc(vla, (vla->unit_size * vla->size) + sizeof(VLARec));
        if(!vla) {
          if(old_vla->grow_factor < 1.001F) {
            printf("VLAExpand-ERR: realloc failed.\n");
            DieOutOfMemory();
          }
        }
      }
    }
    if(vla->auto_zero) {
      start = ((char *) vla) + soffset;
      stop = ((char *) vla) + sizeof(VLARec) + (vla->unit_size * vla->size);
      MemoryZero(start, stop);
    }
  }
  return ((void *) &(vla[1]));
}

void *VLAMalloc(ov_size init_size, ov_size unit_size, unsigned int grow_factor,
                int auto_zero)
{
  VLARec *vla;
  char *start, *stop;
  vla = (VLARec*) mmalloc((init_size * unit_size) + sizeof(VLARec));

  if(!vla) {
    printf("VLAMalloc-ERR: malloc failed\n");
    DieOutOfMemory();
  }
  vla->size = init_size;
  vla->unit_size = unit_size;
  vla->grow_factor = (1.0F + grow_factor * 0.1F);
  vla->auto_zero = auto_zero;
  if(vla->auto_zero) {
    start = ((char *) vla) + sizeof(VLARec);
    stop = ((char *) vla) + sizeof(VLARec) + (vla->unit_size * vla->size);
    MemoryZero(start, stop);
  }
  return ((void *) &(vla[1]));
}

void VLAFree(void *ptr)
{
  VLARec *vla;
  if(!ptr) {
    printf("VLAFree-ERR: tried to free NULL pointer!\n");
    exit(EXIT_FAILURE);
  }
  vla = &(((VLARec *) ptr)[-1]);
  mfree(vla);
}

unsigned int VLAGetSize(const void *ptr)
{
  const VLARec *vla;
  vla = &((VLARec *) ptr)[-1];
  return (vla->size);
}

void *VLANewCopy(const void *ptr)
{
  if(ptr) {                     /* NULL protected */
    const VLARec *vla;
    VLARec *new_vla;
    unsigned int size;
    vla = &((VLARec *) ptr)[-1];
    size = (vla->unit_size * vla->size) + sizeof(VLARec);
    new_vla = (VLARec*) mmalloc(size);
    if(!new_vla) {
      printf("VLACopy-ERR: mmalloc failed\n");
      exit(EXIT_FAILURE);
    } else {
      memcpy(new_vla, vla, size);
    }
    return ((void *) &(new_vla[1]));
  } else {
    return NULL;
  }
}

void *VLASetSize(void *ptr, unsigned int new_size)
{
  VLARec *vla;
  char *start = NULL;
  char *stop;
  unsigned int soffset = 0;
  vla = &((VLARec *) ptr)[-1];
  if(vla->auto_zero) {
    soffset = sizeof(VLARec) + (vla->unit_size * vla->size);
  }
  vla->size = new_size;
  vla = (VLARec*) mrealloc(vla, (vla->unit_size * vla->size) + sizeof(VLARec));
  if(!vla) {
    printf("VLASetSize-ERR: realloc failed.\n");
    DieOutOfMemory();
  }
  if(vla->auto_zero) {
    start = ((char *) vla) + soffset;
    stop = ((char *) vla) + sizeof(VLARec) + (vla->unit_size * vla->size);
    if(start < stop)
      MemoryZero(start, stop);
  }
  return ((void *) &(vla[1]));
}

void *VLADeleteRaw(void *ptr, int index, unsigned int count)
{
  if(ptr) {
    VLARec *vla = ((VLARec *) ptr) - 1;
    ov_size old_size = vla->size;

    /* failsafe range-handling logic */

    if(index<0) {
      if(index < -old_size)
        index = 0;
      else
        index = old_size + 1 + index;
      if(index<0) index = 0;
    }

    if((count+index) > vla->size) {
      count = vla->size - index;
    }
      
    if((index >= 0) && (count > 0) &&
       (index < vla->size) && ((count + index) <= vla->size)) {
      ov_size new_size = old_size - count;
      ov_char *base = (ov_char *) ptr;
      ov_os_memmove(base + index * vla->unit_size,
                    base + (count + index) * vla->unit_size,
                    ((vla->size - index) - count) * vla->unit_size);
      ptr = VLASetSize(ptr,new_size);
    }
  }
  return ptr;
}

void *VLAInsertRaw(void *ptr, int index, unsigned int count)
{
  if(ptr) {
    VLARec *vla = ((VLARec *) ptr) - 1;
    ov_size old_size = vla->size;

    /* failsafe range-handling logic */

    if(index<0) {
      if(index < -old_size)
        index = 0;
      else
        index = old_size + 1 + index;
      if(index<0) index = 0;
    }
    
    if(index > old_size)
      index = old_size;

    if((index >= 0) && (count > 0) && (index <= old_size)) {
      ov_int new_size = old_size + count;

      ptr = VLASetSize(ptr,new_size);
      if(ptr) {
        ov_char *base = (ov_char *) ptr;
        VLARec *vla = ((VLARec *) ptr) - 1;
        ov_os_memmove(base + (index + count) * vla->unit_size,
                base + index * vla->unit_size, (old_size - index) * vla->unit_size);
        if(vla->auto_zero) 
          ov_os_memset(base + index * vla->unit_size, 0, vla->unit_size * count);
      }
    }
  }
  return ptr;
}

void *VLASetSizeForSure(void *ptr, unsigned int new_size)
{
  VLARec *vla;
  char *start = NULL;
  char *stop;
  unsigned int soffset = 0;
  vla = &((VLARec *) ptr)[-1];
  if(vla->auto_zero) {
    soffset = sizeof(VLARec) + (vla->unit_size * vla->size);
  }
  if(new_size < vla->size) {
    vla = (VLARec*) MemoryReallocForSureSafe(vla,
                                   (vla->unit_size * new_size) + sizeof(VLARec),
                                   (vla->unit_size * vla->size) + sizeof(VLARec));
    vla->size = new_size;
  } else {
    vla->size = new_size;
    vla = (VLARec*) mrealloc(vla, (vla->unit_size * vla->size) + sizeof(VLARec));
  }
  if(!vla) {
    printf("VLASetSize-ERR: realloc failed.\n");
    DieOutOfMemory();
  }
  if(vla->auto_zero) {
    start = ((char *) vla) + soffset;
    stop = ((char *) vla) + sizeof(VLARec) + (vla->unit_size * vla->size);
    if(start < stop)
      MemoryZero(start, stop);
  }
  return ((void *) &(vla[1]));
}
