/*
 *  oftc-ircservices: an exstensible and flexible IRC Services package
 *  operserv.c: A C implementation of Operator Services
 *
 *  Copyright (C) 2006 Stuart Walsh and the OFTC Coding department
 *
 *  Some parts:
 *
 *  Copyright (C) 2002 by the past and present ircd coders, and others.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307
 *  USA
 *
 *  $Id$
 */

#include "stdinc.h"

static struct Service *operserv = NULL;

static dlink_node *os_newuser_hook;

static void *os_on_newuser(va_list);

static void m_help(struct Service *, struct Client *, int, char *[]);
static void m_raw(struct Service *, struct Client *, int, char *[]);
static void m_mod_list(struct Service *, struct Client *, int, char *[]);
static void m_mod_load(struct Service *, struct Client *, int, char *[]);
static void m_mod_reload(struct Service *, struct Client *, int, char *[]);
static void m_mod_unload(struct Service *, struct Client *, int, char *[]);
static void m_operserv_notimp(struct Service *, struct Client *, int, char *[]);
static void m_admin_add(struct Service *, struct Client *, int, char *[]);
static void m_admin_list(struct Service *, struct Client *, int, char *[]);
static void m_admin_del(struct Service *, struct Client *, int, char *[]);
static void m_akill_add(struct Service *, struct Client *, int, char *[]);
static void m_akill_list(struct Service *, struct Client *, int, char *[]);
static void m_akill_del(struct Service *, struct Client *, int, char *[]);

static struct ServiceMessage help_msgtab = {
  NULL, "HELP", 0, 0, 2, 0, ADMIN_FLAG, OS_HELP_SHORT, OS_HELP_LONG, m_help
};

static struct ServiceMessage mod_subs[] = {
  { NULL, "LIST", 0, 0, 0, 0, ADMIN_FLAG, OS_MOD_LIST_HELP_SHORT, 
    OS_MOD_LIST_HELP_LONG, m_mod_list },
  { NULL, "LOAD", 0, 1, 1, 0, ADMIN_FLAG, OS_MOD_LOAD_HELP_SHORT, 
    OS_MOD_LOAD_HELP_LONG, m_mod_load },
  { NULL, "RELOAD", 0, 1, 1, 0, ADMIN_FLAG, OS_MOD_RELOAD_HELP_SHORT, 
    OS_MOD_RELOAD_HELP_LONG, m_mod_reload },
  { NULL, "UNLOAD", 0, 1, 1, 0, ADMIN_FLAG, OS_MOD_UNLOAD_HELP_SHORT, 
    OS_MOD_UNLOAD_HELP_LONG, m_mod_unload },
  { NULL, NULL, 0, 0, 0, 0, 0, 0, 0, NULL }
};

static struct ServiceMessage mod_msgtab = {
  mod_subs, "MOD", 0, 1, 1, 0, ADMIN_FLAG, OS_MOD_HELP_SHORT, OS_MOD_HELP_LONG,
  NULL
};

static struct ServiceMessage raw_msgtab = {
  NULL, "RAW", 1, 1, 1, SFLG_NOMAXPARAM, ADMIN_FLAG, OS_RAW_HELP_SHORT, OS_RAW_HELP_LONG, m_raw
};

static struct ServiceMessage admin_subs[] = {
  { NULL, "ADD", 0, 1, 1, 0, ADMIN_FLAG, OS_ADMIN_ADD_HELP_SHORT, 
    OS_ADMIN_ADD_HELP_LONG, m_admin_add },
  { NULL, "LIST", 0, 0, 0, 0, ADMIN_FLAG, OS_ADMIN_LIST_HELP_SHORT, 
    OS_ADMIN_LIST_HELP_LONG, m_admin_list },
  { NULL, "DEL", 0, 1, 1, 0, ADMIN_FLAG, OS_ADMIN_DEL_HELP_SHORT, 
    OS_ADMIN_DEL_HELP_LONG, m_admin_del },
  { NULL, NULL, 0, 0, 0, 0, 0, 0, 0, NULL }
};

static struct ServiceMessage admin_msgtab = {
  admin_subs, "ADMIN", 0, 1, 1, 0, ADMIN_FLAG, OS_ADMIN_HELP_SHORT, 
  OS_ADMIN_HELP_LONG, NULL
};

static struct ServiceMessage akill_subs[] = {
  { NULL, "ADD", 0, 2, 3, SFLG_NOMAXPARAM, OPER_FLAG, OS_AKILL_ADD_HELP_SHORT, 
    OS_AKILL_ADD_HELP_LONG, m_akill_add },
  { NULL, "LIST", 0, 0, 0, 0, OPER_FLAG, OS_AKILL_LIST_HELP_SHORT, 
    OS_AKILL_LIST_HELP_LONG, m_akill_list },
  { NULL, "DEL", 0, 1, 1, 0, OPER_FLAG, OS_AKILL_DEL_HELP_SHORT, 
    OS_AKILL_DEL_HELP_LONG, m_akill_del },
  { NULL, NULL, 0, 0, 0, 0, 0, 0, 0, NULL }
};

static struct ServiceMessage akill_msgtab = {
  akill_subs, "AKILL", 0, 2, 2, 0, ADMIN_FLAG, OS_AKILL_HELP_SHORT, 
  OS_AKILL_HELP_LONG, NULL
};

static struct ServiceMessage set_msgtab = {
  NULL, "SET", 1, 1, 1, 0, ADMIN_FLAG, OS_SET_HELP_SHORT, OS_SET_HELP_LONG,
  m_operserv_notimp
};

INIT_MODULE(operserv, "$Revision$")
{
  operserv = make_service("OperServ");
  clear_serv_tree_parse(&operserv->msg_tree);
  dlinkAdd(operserv, &operserv->node, &services_list);
  hash_add_service(operserv);
  introduce_client(operserv->name);

  load_language(operserv->languages, "operserv.en");

  os_newuser_hook = install_hook(on_newuser_cb, os_on_newuser);

  mod_add_servcmd(&operserv->msg_tree, &help_msgtab);
  mod_add_servcmd(&operserv->msg_tree, &mod_msgtab);
  mod_add_servcmd(&operserv->msg_tree, &raw_msgtab);
  mod_add_servcmd(&operserv->msg_tree, &admin_msgtab);
  mod_add_servcmd(&operserv->msg_tree, &akill_msgtab);
  mod_add_servcmd(&operserv->msg_tree, &set_msgtab);
  mod_add_servcmd(&operserv->msg_tree, &raw_msgtab);
}

CLEANUP_MODULE
{
  uninstall_hook(on_newuser_cb, os_on_newuser);

  serv_clear_messages(operserv);

  unload_languages(operserv->languages);
  ilog(L_DEBUG, "Unloaded operserv");
}

static void
m_help(struct Service *service, struct Client *client,
        int parc, char *parv[])
{
  do_help(service, client, parv[1], parc, parv);
}

static void
m_mod_list(struct Service *service, struct Client *client,
        int parc, char *parv[])
{
  dlink_node *ptr;

  DLINK_FOREACH(ptr, loaded_modules.head)
  {
    struct Module *mod = ptr->data;

    reply_user(service, service, client, OS_MOD_LIST, mod->name, mod->version,
        mod->type == MODTYPE_RUBY ? "Ruby" : "so");
  }

  reply_user(service, service, client, OS_MOD_LIST_END);
}

static void
m_mod_load(struct Service *service, struct Client *client,
            int parc, char *parv[])
{
  char *parm = parv[1];
  char *mbn;

  mbn = basename(parm);

  if (find_module(mbn, 0) != NULL)
  {
    reply_user(service, service, client, OS_MOD_ALREADYLOADED, parm);
    return;
  }

  if (parm == NULL)
  {
    reply_user(service, service, client, 0, "You need to specify the modules name");
    return;
  }

  global_notice(service, "Loading %s by request of %s",
      parm, client->name);
  if (load_module(parm) == 1)
  {
    global_notice(service, "Module %s loaded", parm);
    reply_user(service, service, client, OS_MOD_LOADED, parm);
  }
  else
  {
    global_notice(service, "Module %s could not be loaded!", parm);
    reply_user(service, service, client, OS_MOD_LOADFAIL, parm);
  }
}

static void
m_mod_reload(struct Service *service, struct Client *client,
    int parc, char *parv[])
{
  char *parm = parv[1];
  char *mbn;
  struct Module *module;

  mbn = basename(parm);
  module = find_module(mbn, 0);
  if (module == NULL)
  {
    global_notice(service,
        "Module %s reload requested by %s, but failed because not loaded",
        parm, client->name);
    reply_user(service, service, client, OS_MOD_NOTLOADED, parm, client->name);
    return;
  }
  global_notice(service, "Reloading %s by request of %s", parm, client->name);
  reply_user(service, service, client, OS_MOD_RELOADING, parm, client->name);
  unload_module(module);
  if (load_module(parm) == 1)
  {
    global_notice(service, "Module %s loaded", parm);
    reply_user(service, service, client, OS_MOD_LOADED,parm);
  }
  else
  {
    global_notice(service, "Module %s could not be loaded!", parm);
    reply_user(service, service, client, OS_MOD_LOADFAIL, parm);
  }
}

static void
m_mod_unload(struct Service *service, struct Client *client,
    int parc, char *parv[])
{
  char *parm = parv[1];
  char *mbn;
  struct Module *module;

  mbn = basename(parm);
  module = find_module(mbn, 0);
  if (module == NULL)
  {
    ilog(L_NOTICE, "Module %s unload requested by %s, but failed because not "
        "loaded", parm, client->name);
    reply_user(service, service, client, OS_MOD_UNLOAD_NOTLOADED, parm); 
    return;
  }
  ilog(L_NOTICE, "Unloading %s by request of %s", parm, client->name);
  reply_user(service, service, client, OS_MOD_UNLOAD, parm, client->name);
  unload_module(module);
}

static void
m_raw(struct Service *service, struct Client *client,
    int parc, char *parv[])
{
  char buffer[IRC_BUFSIZE+1];
  int i;

  memset(buffer, 0, sizeof(buffer));
  
  for(i = 1; i <= parc; i++)
  {
    strlcat(buffer, parv[i], sizeof(buffer));
    strlcat(buffer, " ", sizeof(buffer));
  }
  if(buffer[strlen(buffer)-1] == ' ')
    buffer[strlen(buffer)-1] = '\0';
  sendto_server(me.uplink, buffer);
  ilog(L_DEBUG, "Executing RAW: \"%s\"", buffer);
}

static void
m_admin_add(struct Service *service, struct Client *client,
    int parc, char *parv[])
{
  struct Nick *nick = db_find_nick(parv[1]);
  struct Client *target;

  if(nick == NULL)
  {
    reply_user(service, service, client, OS_NICK_NOTREG, parv[1]);
    return;
  }
  nick->admin = TRUE;
  db_set_bool(SET_NICK_ADMIN, nick->id, TRUE);
  reply_user(service, service, client, OS_ADMIN_ADDED, nick->nick);
  free_nick(nick);

  /* Actively enforce the admin add in case the nick is online right now */
  if((target = find_client(parv[1])) != NULL)
  {
    if(target->nickname != NULL)
    {
      target->nickname->admin = TRUE;
      if(IsOper(target))
        target->access = ADMIN_FLAG;
      else
        target->access = IDENTIFIED_FLAG;
    }
  }
}

static void
m_admin_list(struct Service *service, struct Client *client,
    int parc, char *parv[])
{
  char *currnick;
  void *first, *handle;
  int i = 1;

  first = handle = db_list_first(ADMIN_LIST, 0, (void**)&currnick);
  while(handle != NULL)
  {
    reply_user(service, service, client, OS_ADMIN_LIST, i++, currnick);
    handle = db_list_next(handle, ADMIN_LIST, (void **)&currnick);
  }
  if(first != NULL)
  db_list_done(first);

  reply_user(service, service, client, OS_ADMIN_LIST_END);
}

static void
m_admin_del(struct Service *service, struct Client *client,
    int parc, char *parv[])
{
  struct Nick *nick;
  struct Client *target;
    
  nick = db_find_nick(parv[1]);
    
  if(nick == NULL || !(nick->admin))
  {
    reply_user(service, service, client, OS_ADMIN_NOTADMIN, parv[1]);
    return;
  }
  reply_user(service, service, client, OS_ADMIN_DEL, nick->nick);
  nick->admin = FALSE;
  db_set_bool(SET_NICK_ADMIN, nick->id, FALSE);

  free_nick(nick);

  /* Actively enforce the admin removal in case the nick is online right now */
  if((target = find_client(parv[1])) != NULL)
  {
    if(target->nickname != NULL)
    {
      target->nickname->admin = FALSE;
      if(IsOper(target))
        target->access = OPER_FLAG;
      else
        target->access = IDENTIFIED_FLAG;
    }
  }
}

/* AKILL ADD [+duration] user@host reason */
static void
m_akill_add(struct Service *service, struct Client *client,
    int parc, char *parv[])
{
  struct ServiceBan *akill, *tmp;
  char reason[IRC_BUFSIZE+1];
  int para_start = 2;
  char *mask = parv[1];
  char duration_char = '\0';
  int duration = -1;
  int input_dur;

  if(*parv[1] == '+')
  {
    char *ptr = parv[1];

    para_start = 3;
    mask = parv[2];
    parc--;

    ptr++;
    while(*ptr != '\0')
    {
      if(!IsDigit(*ptr))
      {
        duration_char = *ptr;
        *ptr = '\0';
        duration = atoi(parv[1]);
        input_dur = duration;
        break;
      }
      ptr++;
    }
  }

  if(duration != -1)
  {
    switch(duration_char)
    {
      case 'm':
        duration *= 60;
        break;
      case 'h':
        duration *= 3600;
        break;
      case 'd':
      case '\0': /* default is days */
        duration *= 86400; 
        duration_char = 'd';
        break;
      default:
        reply_user(service, service, client, OS_AKILL_BAD_DURATIONCHAR, 
            duration_char);
        return;
    }
  }
  else 
  {
    duration_char = 'd';
    input_dur = duration = ServicesInfo.def_akill_dur;
    input_dur /= 86400;
  }

  if(duration == -1)
    duration = 0;

  if((tmp = db_find_akill(mask)) != NULL)
  {
    reply_user(service, service, client, OS_AKILL_ALREADY, mask);
    free_serviceban(tmp);
    return;
  }

  if(!valid_wild_card(mask))
  {
    reply_user(service, service, client, OS_AKILL_TOO_WILD, 
        ServicesInfo.min_nonwildcard);
    return;
  }

  join_params(reason, parc-1, &parv[para_start]);

  akill = akill_add(service, client, mask, reason, duration);

  if(akill == NULL)
  {
    reply_user(service, service, client, OS_AKILL_ADDFAIL, mask);
    return;
  }

  reply_user(service, service, client, OS_AKILL_ADDOK, mask);
  free_serviceban(akill);
}

static void
m_akill_list(struct Service *service, struct Client *client,
    int parc, char *parv[])
{
  struct ServiceBan *akill;
  void *handle, *first;
  char setbuf[TIME_BUFFER + 1];
  char durbuf[TIME_BUFFER + 1];
  int i = 1;

  first = handle = db_list_first(AKILL_LIST, 0, (void**)&akill);
  while(handle != NULL)
  {
    char *setter = db_get_nickname_from_id(akill->setter);

    strtime(client, akill->time_set, setbuf);
    strtime(client, akill->time_set + akill->duration, durbuf);

    reply_user(service, service, client, OS_AKILL_LIST, i++, akill->mask, 
        akill->reason, setter, setbuf, akill->duration == 0 ? "N/A" : durbuf);
    free_serviceban(akill);
    MyFree(setter);
    handle = db_list_next(handle, AKILL_LIST, (void**)&akill);
  }
  if(first)
    db_list_done(first);

  free_serviceban(akill);

  reply_user(service, service, client, OS_AKILL_LIST_END);
}

static void
m_akill_del(struct Service *service, struct Client *client,
    int parc, char *parv[])
{
  int ret = 0;
  struct ServiceBan *akill;

  if((akill = db_find_akill(parv[1])) == NULL)
  {
    reply_user(service, service, client, OS_AKILL_DEL, 0);
    return;
  }
  
  ret = db_list_del(DELETE_AKILL, 0, parv[1]);
  remove_akill(service, akill);
  reply_user(service, service, client, OS_AKILL_DEL, ret);
}

static void m_operserv_notimp(struct Service *service, struct Client *client, 
    int parc, char *parv[])
{
  reply_user(service, service, client, 0, "This isnt implemented yet.");
}

static void *
os_on_newuser(va_list args)
{
  struct Client *newuser = va_arg(args, struct Client*);
  
  if(IsMe(newuser->from))
    return pass_callback(os_newuser_hook, newuser);

  enforce_matching_serviceban(operserv, NULL, newuser); 

  return pass_callback(os_newuser_hook, newuser);
}
