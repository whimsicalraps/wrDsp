#include "wrBuffer.h"
#include <stdlib.h>

buffer_t* buffer_init(int* p_byte)
{
  buffer_t* self = malloc(sizeof(p_byte) + sizeof(buffer_state_t));

  self->byte_array = p_byte;
  self->p_access = &self->byte_array;

  return self;
}

void buffer_deinit(buffer_t* self)
{
  free(self);
}
