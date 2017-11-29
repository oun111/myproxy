/* Copyright (c) 2000, 2010, Oracle and/or its affiliates. All rights reserved.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301  USA */

/*
  Code for handling dubble-linked lists in C
*/

/*
  2016.6.22: yzhou modified
*/

//#include "mysys_priv.h"
#include "list.h"



	/* Add a element to start of list */

#if 0
LIST *list_add(LIST *root, LIST *element)
{
  //DBUG_ENTER("list_add");
  //DBUG_PRINT("enter",("root: 0x%lx  element: 0x%lx", (long) root, (long) element));
  if (root)
  {
    if (root->prev)			/* If add in mid of list */
      root->prev->next= element;
    element->prev=root->prev;
    root->prev=element;
  }
  else
    element->prev=0;
  element->next=root;
  //DBUG_RETURN(element);			/* New root */
  return element ;
}
#else
/* yzhou: add to end of list */
LIST *list_add(LIST *root, LIST *element)
{
  //DBUG_ENTER("list_add");
  //DBUG_PRINT("enter",("root: 0x%lx  element: 0x%lx", (long) root, (long) element));
  if (root)
  {
    if (root->next) { 	/* If add in mid of list */
      root->next->prev= element;
      element->next=root->next;
    }
    element->prev=root;
    root->next=element;
  }
  else
    element->next=0;
  //DBUG_RETURN(element);			/* New root */
  return element ;
}
#endif


LIST *list_delete(LIST *root, LIST *element)
{
  if (element->prev)
    element->prev->next=element->next;
  else
    root=element->next;
  if (element->next)
    element->next->prev=element->prev;
  return root;
}


#if 0
void list_free(LIST *root, uint16_t free_data)
{
  LIST *next;
  while (root)
  {
    next=root->next;
    if (free_data)
      my_free(root->data);
    my_free(root);
    root=next;
  }
}

LIST *list_cons(void *data, LIST *list)
{
  LIST *new_charset=(LIST*) my_malloc(sizeof(LIST),MYF(MY_FAE));
  if (!new_charset)
    return 0;
  new_charset->data=data;
  return list_add(list,new_charset);
}
#endif


LIST *list_reverse(LIST *root)
{
  LIST *last;

  last=root;
  while (root)
  {
    last=root;
    root=root->next;
    last->next=last->prev;
    last->prev=root;
  }
  return last;
}

/*uint*/ unsigned int list_length(LIST *list)
{
  /*uint*/unsigned int count;
  for (count=0 ; list ; list=list->next, count++) ;
  return count;
}

#if 0
int list_walk(LIST *list, list_walk_action action, /*uchar*/unsigned char* argument)
{
  int error=0;
  while (list)
  {
    if ((error = (*action)(list->data,argument)))
      return error;
    list=list_rest(list);
  }
  return 0;
}
#endif
