#include <pthread.h>
#include <memory.h>

#include "vkapi.h"
#include "vk_setting.h"

#define MAX(a,b) ((a) > (b) ? (a) : (b))

typedef struct list_s {
  vkapi_message_new_object *object;
  struct list_s *next;
} list_t;

struct queue {
  list_t *head, *tail;
};

struct queue *queue_pool = NULL;
static size_t maxium_tasks_in_queue;
static size_t queue_tasks_in_queue;

int queue_is_empty() {
  return (queue_pool->head == NULL && queue_pool->tail == NULL);
}

size_t queue_maxium_tasks_in_queue()
{
  return maxium_tasks_in_queue;
}

void add_queue(cJSON *update_block) {
  list_t *element = malloc(sizeof(list_t));
  vkapi_message_new_object *x = malloc(sizeof(vkapi_message_new_object));
  x->text = string_init();

  queue_tasks_in_queue++;

  maxium_tasks_in_queue = MAX(queue_tasks_in_queue, maxium_tasks_in_queue);

  cJSON *object = cJSON_GetObjectItem(update_block, "object");
  cJSON *peer_id = cJSON_GetObjectItem(object, "peer_id");

  x->peer_id = peer_id->valueint;

  if(VK_GROUP_ID == x->peer_id * -1)
    {
      printf("skip group message\n");
      free(x);
      free(element);
      cJSON_Delete(update_block);
      return;
    }

  cJSON *from_id = cJSON_GetObjectItem(object, "from_id");

  x->from_id = from_id->valueint;

  cJSON *text_string = cJSON_GetObjectItem(object, "text");

  if(text_string)
    string_copy(x->text, cJSON_GetStringValue(text_string));

  cJSON *attachments = cJSON_GetObjectItem(object, "attachments");

  x->attachmens = NULL;

  if(cJSON_GetArraySize(attachments) > 0)
    x->attachmens = cJSON_Duplicate(attachments, true);

  cJSON_Delete(update_block);

  element->object = x;
  element->next = NULL;

  if (queue_pool->head == NULL) {
      queue_pool->head = element;
      queue_pool->tail = element;
    } else {
      list_t *old_tail = queue_pool->tail;
      old_tail->next = element;
      queue_pool->tail = element;
    }
}

vkapi_message_new_object *get_queue() {
  list_t *head = queue_pool->head;

  if(head == NULL)
    return NULL;

  queue_tasks_in_queue--;

  queue_pool->head = head->next;
  vkapi_message_new_object *value = head->object;

  free(head);
  return(value);
}

void queue_init()
{
  queue_pool = malloc(sizeof(struct queue));
  queue_tasks_in_queue = 0;
  maxium_tasks_in_queue = 0;
  queue_pool->head = NULL;
  queue_pool->tail = NULL;
}
