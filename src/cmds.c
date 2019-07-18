#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <memory.h>
#include <pthread.h>
#include <base64_decoder.h>

#include "cmds.h"
#include "crc32_hash.h"
#include "vkapi.h"
#include "va_utils.h"

#include <cJSON.h>

#include "base64_decoder.h"

typedef struct
{
  const char	*string;
  void	(*function)(vkapi_object *object, vkapi_message_new_object *message, int argc, char **argv, const char *args);
} cmds_t;

typedef struct
{
  unsigned int hash;
  void	(*function)(vkapi_object *object, vkapi_message_new_object *message, int argc, char **argv, const char *args);
} cmds_hashs_t;

typedef struct
{
  const char *name;
} cmds_names_t;

typedef struct
{ 
  unsigned int hash;
} cmds_name_hashs_t;

cmds_hashs_t *cached_cmds = NULL;

cmds_name_hashs_t *cached_names = NULL;

#define ARRAY_LENGHT(x) (sizeof(x)/sizeof(x[0])) - 1

void cmd_help(vkapi_object *object, vkapi_message_new_object *message, int argc, char **argv, const char *args)
{
  vkapi_send_message(object, message->peer_id, "Обычно тут пишут помощь. Кто бы мне помог ААААААААААААААААААААААААААААААААА");
}

void cmd_ping(vkapi_object *object, vkapi_message_new_object *message, int argc, char **argv, const char *args)
{
  vkapi_send_message(object, message->peer_id, "Pong");
}

void cmd_base64_encode(vkapi_object *object, vkapi_message_new_object *message, int argc, char **argv, const char *args)
{
  struct string *s = NULL;

  if(argc < 1)
    {
      vkapi_send_message(object, message->peer_id, "Использование: b64e <строка>");
      return;
    }

  s = init_string();
  base64_encode((const unsigned char *)args, (unsigned char *)s->ptr, strlen(args), 1);
  vkapi_send_message(object, message->peer_id, va("Закодированная строка: %s", s->ptr));
  destroy_string(s);
}

void cmd_base64_decode(vkapi_object *object, vkapi_message_new_object *message, int argc, char **argv, const char *args)
{
  struct string *s = NULL;

  if(argc < 1)
      {
  	vkapi_send_message(object, message->peer_id, "Использование: b64d <строка>");
	return;
      }

  s = init_string();
  base64_decode((const unsigned char *)args, (unsigned char *)s->ptr, strlen(args));
  vkapi_send_message(object, message->peer_id, va("Декодированная строка: %s", s->ptr));
  destroy_string(s);
}

static cmds_t commands[] = {
  { "помощь", cmd_help },
  { "ping", cmd_ping },
  { "b64e", cmd_base64_encode },
  { "b64d", cmd_base64_decode },
  { NULL, NULL }
};

static cmds_names_t names[] = {
  { "Максбот" },
  { "максбот" },
  { "Максимбот" },
  { "максимбот" },
  { NULL }
};

static size_t static_names = ARRAY_LENGHT(names);

static size_t static_commands = ARRAY_LENGHT(commands);

vkapi_bool cmd_is_bot_name(const char *name)
{
  size_t name_len = strlen(name);

  if(name_len > 32)
    return false;

  unsigned int name_hash = crc32_calc((const unsigned char*)name, name_len);

  for(size_t i = 0; i < static_names; i++) {
      if(cached_names[i].hash == name_hash)
	{
  	return true;
	}
    }

  return false;
}

cmds_hashs_t *cmd_get_command(const char *command_name)
{
  size_t name_len = strlen(command_name);

  if(name_len > 32)
    return NULL;

  unsigned int name_hash = crc32_calc((const unsigned char*)command_name, name_len);

  printf("%s : %X\n", command_name, name_hash);

  for(size_t i = 0; i < static_names; i++) {
      if(cached_cmds[i].hash == name_hash)
	{
	  return &cached_cmds[i];
	}
    }

  return NULL;
}

void cmd_handle(vkapi_object *object, vkapi_message_new_object *message)
{
  char *saveptr = NULL;
  char *argv[256] = { NULL };
  char *token = NULL;
  struct string *s = dublicate_string(message->text);
  struct string *args_s = init_string();

  if(message->text->len == 0)
    return;

   token = strtok_r(s->ptr, " ", &saveptr);

   if(!token)
     goto end;

   if(!cmd_is_bot_name(token))
    {
      goto end;
    }

   int i = 0;
   while (token != NULL) {
       token = strtok_r(NULL, " ", &saveptr);
        if(token)
	  argv[i++] = token;
     }

   if(!argv[0])
     {
     goto dada;
     } else {
       for(int c = 1; c < i; c++ )
	 {
	   if(c == 1)
	   strncat_to_string(args_s, argv[c], strlen(argv[c]));
	   else
	     {
	       char *token_space = va(" %s", argv[c]);
	       strncat_to_string(args_s, token_space, strlen(token_space));
	     }

	   s->ptr[s->len] = '\0';
	 }
     }

   printf("Try to call cmd %s\n", argv[0]);
   cmds_hashs_t *cmd = cmd_get_command(argv[0]);

   if(cmd)
     {
       if(cmd->function)
	 cmd->function(object, message, i - 1, argv, args_s->ptr);
       goto end;
     } else {
       goto not_found;
     }

dada:
    vkapi_send_message(object, message->peer_id, "Да-да?\n Для того чтобы узнать команды используйте помощь.");
    destroy_string(s);
    destroy_string(args_s);
    return;


not_found:
   vkapi_send_message(object, message->peer_id, "Команда не найдена\n Для того чтобы узнать команды используйте помощь.");
   destroy_string(s);
   destroy_string(args_s);
   return;

end:
   destroy_string(s);
   destroy_string(args_s);
   return;
}

void cmd_calculate_cmd_hashes()
{
  cached_cmds = (cmds_hashs_t*)calloc(static_commands, sizeof(cmds_hashs_t));

  if(cached_cmds == NULL)
    {
      printf("Error while allocated memory for commands hashs\n");
      exit(EXIT_FAILURE);
    }

  printf("Static commands in bot %lu ( static cmds_t commands[] )\n", static_commands);

  for(size_t i = 0; i < static_commands; i++)
    {
      if(commands[i].string == NULL)
	break;

      cached_cmds[i].hash = crc32_calc((const unsigned char*)commands[i].string, strlen(commands[i].string));
      cached_cmds[i].function = commands[i].function;

      printf("%s : hash %X\n", commands[i].string, cached_cmds[i].hash);

    }
}

void cmd_calculate_name_hashes()
{
  cached_names = (cmds_name_hashs_t*)calloc(static_names, sizeof(cmds_name_hashs_t));

  if(cached_names == NULL)
    {
      printf("Error while allocated memory for commands hashs\n");
      exit(EXIT_FAILURE);
    }

  printf("Static names in bot %lu ( static cmds_names_t names[] )\n", static_names);

  for(size_t i = 0; i < static_names; i++)
    {
      if(names[i].name == NULL)
	break;

      cached_names[i].hash = crc32_calc((unsigned char*)names[i].name, strlen(names[i].name));
      printf("%s : hash %X\n", names[i].name, cached_names[i].hash);
    }
}

void cmd_calculate_hashes()
{
  cmd_calculate_cmd_hashes();
  cmd_calculate_name_hashes();
}

void cmd_handler_init(const char *name)
{
  cmd_calculate_hashes();
}
