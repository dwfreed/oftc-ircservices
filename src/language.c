/*
 *  oftc-ircservices: an exstensible and flexible IRC Services package
 *  language.c: Language file functions
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
 *  $Id$
 */

#include "stdinc.h"

void
load_language(struct Service *service, const char *langfile)
{
  FBFILE *file;
  char buffer[256];
  char *s;
  int lang, i = 0;

  snprintf(buffer, sizeof(buffer), "%s/%s.lang", LANGPATH, langfile);

  if((file = fbopen(buffer, "r")) == NULL)
  {
    ilog(L_DEBUG, "Failed to open language file %s for service %s(%s)", langfile,
        service->name, buffer);
    return;
  }
  
  /* Read the first line which tells us which language this is */
  fbgets(buffer, sizeof(buffer), file);
  if((s = strchr(buffer, ' ')) == NULL)
  {
    ilog(L_DEBUG, "Language file %s for service %s is invalid", langfile,
        service->name);
    return;
  }

  *s++ = '\0';
  lang = atoi(buffer);
 
  if(s[strlen(s) - 1] == '\n')
    s[strlen(s) - 1] = '\0';
  
  DupString(service->language_table[lang][i], s);
  
  ilog(L_DEBUG, "Loading language %d(%s) for service %s", lang, langfile,
      service->name);

  while(fbgets(buffer, sizeof(buffer), file) != NULL)
  {
    char *ptr;

    if(buffer[0] != '\t')
    {
      i++;
      continue;
    }

    s = buffer;
    s++;

    if(service->language_table[lang][i] != NULL)
    {
      ptr = MyMalloc(strlen(service->language_table[lang][i]) + strlen(s)+1);
      strcpy(ptr, service->language_table[lang][i]);
      MyFree(service->language_table[lang][i]);
      strcat(ptr, s);
    }
    else
    {
      ptr = MyMalloc(strlen(s)+1);
      strcpy(ptr, s);
    }
    service->language_table[lang][i] = ptr;
  }
}
