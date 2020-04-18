/**************************************************************************
 *
 * Copyright 2009 VMware, Inc.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/


/**
 * Scene queue.  We'll use two queues.  One contains "full" scenes which
 * are produced by the "setup" code.  The other contains "empty" scenes
 * which are produced by the "rast" code when it finishes rendering a scene.
 */

#include "util/u_ringbuffer.h"
#include "util/u_memory.h"
#include "lp_scene_queue.h"



#define MAX_SCENE_QUEUE 4

struct scene_packet {
   struct util_packet header;
   struct lp_scene *scene;
};

/**
 * A queue of scenes
 */
struct lp_scene_queue
{
   struct util_ringbuffer *ring;
};



/** Allocate a new scene queue */
struct lp_scene_queue *
lp_scene_queue_create(void)
{
   struct lp_scene_queue *queue = CALLOC_STRUCT(lp_scene_queue);
   if (queue == NULL)
      return NULL;

   queue->ring = util_ringbuffer_create( MAX_SCENE_QUEUE * 
                                         sizeof( struct scene_packet ) / 4);
   if (queue->ring == NULL)
      goto fail;

   return queue;

fail:
   FREE(queue);
   return NULL;
}


/** Delete a scene queue */
void
lp_scene_queue_destroy(struct lp_scene_queue *queue)
{
   util_ringbuffer_destroy(queue->ring);
   FREE(queue);
}


/** Remove first lp_scene from head of queue */
struct lp_scene *
lp_scene_dequeue(struct lp_scene_queue *queue, boolean wait)
{
   struct scene_packet packet;
   enum pipe_error ret;

   packet.scene = NULL;

   ret = util_ringbuffer_dequeue(queue->ring,
                                 &packet.header,
                                 sizeof packet / 4,
                                 wait );
   if (ret != PIPE_OK)
      return NULL;

   return packet.scene;
}


/** Add an lp_scene to tail of queue */
void
lp_scene_enqueue(struct lp_scene_queue *queue, struct lp_scene *scene)
{
   struct scene_packet packet;

   packet.header.dwords = sizeof packet / 4;
   packet.header.data24 = 0;
   packet.scene = scene;

   util_ringbuffer_enqueue(queue->ring, &packet.header);
}





