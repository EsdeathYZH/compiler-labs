/*
 * temp.c - functions to create and manipulate temporary variables which are
 *          used in the IR tree representation before it has been determined
 *          which variables are to go into registers.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"
#include "symbol.h"
#include "temp.h"
#include "table.h"

struct Temp_temp_ {int num;};

int Temp_int(Temp_temp t)
{
	return t->num;
}

string Temp_labelstring(Temp_label s)
{return S_name(s);
}

static int labels = 0;

Temp_label Temp_newlabel(void)
{char buf[100];
 sprintf(buf,"L%d",labels++);
 return Temp_namedlabel(String(buf));
}

/* The label will be created only if it is not found. */
Temp_label Temp_namedlabel(string s)
{return S_Symbol(s);
}

static int temps = 100;

Temp_temp Temp_newtemp(void)
{Temp_temp p = (Temp_temp) checked_malloc(sizeof (*p));
 p->num=temps++;
 {char r[16];
  sprintf(r, "%d", p->num);
  Temp_enter(Temp_name(), p, String(r));
 }
 return p;
}



struct Temp_map_ {TAB_table tab; Temp_map under;};


Temp_map Temp_name(void) {
 static Temp_map m = NULL;
 if (!m) m=Temp_empty();
 return m;
}

Temp_map newMap(TAB_table tab, Temp_map under) {
  Temp_map m = checked_malloc(sizeof(*m));
  m->tab=tab;
  m->under=under;
  return m;
}

Temp_map Temp_empty(void) {
  return newMap(TAB_empty(), NULL);
}

Temp_map Temp_layerMap(Temp_map over, Temp_map under) {
  if (over==NULL)
      return under;
  else return newMap(over->tab, Temp_layerMap(over->under, under));
}

void Temp_enter(Temp_map m, Temp_temp t, string s) {
  assert(m && m->tab);
  TAB_enter(m->tab,t,s);
}

string Temp_look(Temp_map m, Temp_temp t) {
  string s;
  assert(m && m->tab);
  s = TAB_look(m->tab, t);
  if (s) return s;
  else if (m->under) return Temp_look(m->under, t);
  else return NULL;
}

Temp_tempList Temp_TempList(Temp_temp h, Temp_tempList t) 
{Temp_tempList p = (Temp_tempList) checked_malloc(sizeof (*p));
 p->head=h; p->tail=t;
 return p;
}

Temp_labelList Temp_LabelList(Temp_label h, Temp_labelList t)
{Temp_labelList p = (Temp_labelList) checked_malloc(sizeof (*p));
 p->head=h; p->tail=t;
 return p;
}

static FILE *outfile;
void showit(Temp_temp t, string r) {
  fprintf(outfile, "t%d -> %s\n", t->num, r);
}

void Temp_dumpMap(FILE *out, Temp_map m) {
  outfile=out;
  TAB_dump(m->tab,(void (*)(void *, void*))showit);
  if (m->under) {
     fprintf(out,"---------\n");
     Temp_dumpMap(out,m->under);
  }
}

bool Temp_inList(Temp_tempList list, Temp_temp temp){
    while(list){
        if(list->head == temp){
            return TRUE;
        }
        list = list->tail;
    }
    return FALSE;
}

/* unorder list implementation - a naive version */

//list1 U list2
Temp_tempList Temp_unionList(Temp_tempList list1, Temp_tempList list2){
    Temp_tempList result = list1;
    while(list2){
        if(!Temp_inList(list1, list2->head)){
            result = Temp_TempList(list2->head, result);
        }
        list2 = list2->tail;
    }
    return result;
}

//list1 ^ list2
Temp_tempList Temp_intersectList(Temp_tempList list1, Temp_tempList list2){
    Temp_tempList result = NULL;
    while(list2){
        if(Temp_inList(list1, list2->head)){
            result = Temp_TempList(list2->head, result);
        }
        list2 = list2->tail;
    }
    return result;
}

//list1 - list2
Temp_tempList Temp_exclusiveList(Temp_tempList list1, Temp_tempList list2){
    Temp_tempList result = NULL;
    while(list1){
        if(!Temp_inList(list2, list1->head)){
            result = Temp_TempList(list1->head, result);
        }
        list1 = list1->tail;
    }
    return result;
}

bool Temp_isSameList(Temp_tempList list1, Temp_tempList list2){
  return (Temp_exclusiveList(list1, list2) == NULL && Temp_exclusiveList(list2, list1) == NULL);
}


Temp_tempList Temp_insertTemp(Temp_tempList list, Temp_temp temp){
    return Temp_TempList(temp, list);
}

Temp_tempList Temp_deleteTemp(Temp_tempList list, Temp_temp temp){
    if(!list) return list;
    if(list->head == temp){
      return list->tail;
    }
    list->tail = Temp_deleteTemp(list->tail, temp);
    return list;
}

Temp_tempList Temp_copyFrom(Temp_tempList origin){
    Temp_tempList result = NULL;
    Temp_tempList* listPtr = &result;
    while(origin){
      (*listPtr) = Temp_TempList(origin->head, NULL);
      listPtr = &((*listPtr)->tail);
      origin = origin->tail;
    }
    return result;
}

/* order list implementation - a less-naive version */

//list1 U list2
// Temp_tempList unionTempList(Temp_tempList list1, Temp_tempList list2){
//     Temp_tempList result = NULL;
//     Temp_tempList* listPtr = &result;

//     while(list1 && list2){
//       if(list1->head > list2->head){
//         *listPtr = Temp_TempList(list1->head, NULL);
//         list1 = list1->tail;
//         listPtr = &(*listPtr)->tail;
//       }else if(list1->head < list2->head){
//         *listPtr = Temp_TempList(list2->head, NULL);
//         list2 = list2->tail;
//         listPtr = &(*listPtr)->tail;
//       }else{
//         *listPtr = Temp_TempList(list1->head, NULL);
//         list1 = list1->tail;
//         list2 = list2->tail;
//         listPtr = &(*listPtr)->tail;
//       }
//     }

//     if(list1){
//       *listPtr = list1;
//     }
//     if(list2){
//       *listPtr = list2;
//     }

//     return result;
// }

// //list1 ^ list2
// Temp_tempList intersectTempList(Temp_tempList list1, Temp_tempList list2){
//     Temp_tempList result = NULL;
//     Temp_tempList* listPtr = &result;

//     while(list1 && list2){
//       if(list1->head > list2->head){
//         list1 = list1->tail;
//       }else if(list1->head < list2->head){
//         list2 = list2->tail;
//       }else{
//         *listPtr = Temp_TempList(list1->head, NULL);
//         list1 = list1->tail;
//         list2 = list2->tail;
//         listPtr = &(*listPtr)->tail;
//       }
//     }
//     return result;
// }

// //list1 - list2
// Temp_tempList exclusiveTempList(Temp_tempList list1, Temp_tempList list2){
//     Temp_tempList result = list1;
//     Temp_tempList* listPtr = &result;
//     while(list2 && (*listPtr)){
//         if((*listPtr)->head > list2->head){
//           list2 = list2->tail;
//         }else if((*listPtr)->head < list2->head){
//           listPtr = &(*listPtr)->tail;
//         }else{
//           *listPtr = (*listPtr)->tail;
//         }
//     }
//     return result;
// }

// bool isSameTempList(Temp_tempList list1, Temp_tempList list2){
//   while(list1 && list2){
//     if(list1->head != list2->head){
//       return FALSE;
//     }else{
//       list1 = list1->tail;
//       list2 = list2->tail;
//     }
//   }
//   if(list1 || list2){
//     return FALSE;
//   }
//   return TRUE;
// }

// Temp_tempList sortTempList(Temp_tempList list){
//   if(list == NULL) return NULL;
//   return insertToTemplist(sortTempList(list->tail), list->head);
// }

// Temp_tempList insertToTemplist(Temp_tempList list, Temp_temp temp){
//   if(!list){
//     return Temp_TempList(temp, NULL);
//   }

//   if(list->head > temp){
//     return Temp_TempList(temp, list);
//   }
//   if(list->head == temp){
//     return list;
//   }

//   Temp_tempList tempList = list;
//   while(1){
//     if(!tempList->tail){
//       tempList->tail = Temp_TempList(temp, NULL);
//       return list;
//     }
//     if(tempList->tail->head < temp){
//       tempList = tempList->head;
//     }else if(tempList->tail->head == temp){
//       return list;
//     }else{
//       tempList->tail = Temp_TempList(temp, tempList->tail);
//       return list;
//     }
//   }
// }
