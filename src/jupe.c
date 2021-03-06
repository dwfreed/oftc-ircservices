/*
 *  oftc-ircservices: an extensible and flexible IRC Services package
 *  jup.c - jupe related functions
 *
 *  Copyright (C) 2006 Stuart Walsh and the OFTC Coding department
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
 *  $Id: dbm.c 1260 2007-12-07 08:53:17Z swalsh $
 */

#include "stdinc.h"
#include "jupe.h"
#include "dbm.h"
#include "chanserv.h"
#include "language.h"
#include "parse.h"
#include "nickname.h"
#include "interface.h"

static struct JupeEntry *
row_to_jupe(row_t *row)
{
  struct JupeEntry *jupe;

  jupe = MyMalloc(sizeof(struct JupeEntry));
  jupe->id = atoi(row->cols[0]);
  DupString(jupe->name, row->cols[1]);
  DupString(jupe->reason, row->cols[2]);
  jupe->setter = atoi(row->cols[3]);

  return jupe;
}

void
free_jupe_list(dlink_list *list)
{
  dlink_node *ptr, *next;
  struct JupeEntry *jupe;

  ilog(L_DEBUG, "Freeing jupe list %p length %lu", list, 
      dlink_list_length(list));

  DLINK_FOREACH_SAFE(ptr, next, list->head)
  {
    jupe = (struct JupeEntry *)ptr->data;

    free_jupeentry(jupe);
    dlinkDelete(ptr, list);
    free_dlink_node(ptr);
  }
}

int
jupe_list(dlink_list *list)
{
  result_set_t *results;
  struct JupeEntry *jupe;
  int error, i;

  memset(list, 0, sizeof(dlink_list));

  results = db_execute(GET_JUPES, &error, "");
  if(error != 0)
  {
    return 0;
  }

  if(results == NULL)
  {
    return 0;
  }

  for(i = 0; i < results->row_count; i++)
  {
    jupe = row_to_jupe(&results->rows[i]);
    dlinkAdd(jupe, make_dlink_node(), list);
  }

  db_free_result(results);

  return dlink_list_length(list);
}

struct JupeEntry *
jupe_find(const char *name)
{
  result_set_t *results;
  row_t *row;
  struct JupeEntry *jupe;
  int error;

  results = db_execute(FIND_JUPE, &error, "s", name);

  if(results == NULL && error != 0)
  {
    ilog(L_CRIT, "jupe_find: database error %d", error);
    return NULL;
  }
  else if(results == NULL)
    return NULL;

  if(results->row_count == 0)
    return NULL;

  row = &results->rows[0];
  jupe = row_to_jupe(row);
  db_free_result(results);

  return jupe;
}

int
jupe_delete(const char *name)
{
  int ret;

  ret = db_execute_nonquery(DELETE_JUPE_NAME, "s", name);

  if(ret == -1)
    return FALSE;

  return TRUE;
}

int
jupe_add(struct JupeEntry *entry)
{
  int ret;

  ret = db_execute_nonquery(INSERT_JUPE, "iss", &entry->setter, entry->name, entry->reason);

  if(ret == -1)
    return FALSE;

  return TRUE;
}
