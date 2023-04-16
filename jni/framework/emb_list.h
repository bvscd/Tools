#ifndef _EMB_LIST_DEFINED
#define _EMB_LIST_DEFINED

#include "emb_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
 *   Double linked list API
 */

/*
 * Double linked list entry.
 *
 */
typedef struct list_entry_s {
   struct list_entry_s *next;    /* next entry                               */ 
   struct list_entry_s *prev;    /* prev entry                               */
} list_entry_t;

/*@@list_init_head
 *
 * Initializes list head
 *
 * C/C++ Syntax:   
 * void 
 *    list_init_head(                                            
 *       list_entry_t*   IN OUT   head);
 *
 * Parameters:     head           pointer to a list head
 *
 * Return:         none
 *
 */
#define /* void */ list_init_head(                                            \
                      /* list_entry_t*   IN OUT */   head)                    \
       { list_init_entry((head)); }   

/*@@list_init_entry
 *
 * Initializes list entry
 *
 * C/C++ Syntax:   
 * void 
 *    list_init_entry(                                            
 *       list_entry_t*   IN OUT   entry);
 *
 * Parameters:     entry          pointer to a list entry
 *
 * Return:         none
 *
 */
#define /* void */ list_init_entry(                                           \
                      /* list_entry_t*   IN OUT */   entry)                   \
       {                                                                      \
          assert((entry) != NULL);                                            \
          (entry)->prev = (entry);                                            \
          (entry)->next = (entry);                                            \
       }

/*@@list_is_empty
 *
 * Is a list empty
 *
 * C/C++ Syntax:   
 * bool 
 *    list_is_empty(                                            
 *       list_entry_t*   IN OUT   head);
 *
 * Parameters:     head           pointer to the head of a list
 *
 * Return:         true           if list is empty
 *                 false          otherwise
 *
 */
#define /* bool */ list_is_empty(                                             \
                      /* list_entry_t*   IN */   head)                        \
       ( (head)->next == (head) )

/*@@list_next
 *
 * Gets a next entry in the list
 *
 * C/C++ Syntax:   
 * list_entry_t* 
 *    list_next(                                            
 *       list_entry_t*   IN OUT   entry);
 *
 * Parameters:     entry          pointer to a list entry
 *
 * Return:         pointer to a next list entry
 *
 */
#define /* list_entry_t* */ list_next(                                        \
                               /* list_entry_t*   IN */   entry)              \
       ( (entry)->next )

/*@@list_prev
 *
 * Gets a previous entry of the list.
 *
 * C/C++ Syntax:   
 * list_entry_t* 
 *    list_prev(                                            
 *       list_entry_t*   IN OUT   entry);
 *
 * Parameters:     entry          pointer to a list entry
 *
 * Return:         pointer to a previous list entry
 *
 */
#define /* list_entry_t* */ list_prev(                                        \
                               /* list_entry_t*   IN */   entry)              \
       ( (entry)->prev )

/*@@list_for_each
 *
 * Loops for each list entry
 *
 * C/C++ Syntax:   
 * list_for_each(                                             
 *    list_entry_t*    IN       from,    
 *    list_entry_t**   IN OUT   pentry) 
 * {
 *    ...
 * }
 *
 * Parameters:     from           pointer to list head
 *                 entry          pointer to a list entry pointer (iterator)
 *
 * Return:         none
 *
 */
#define /* none */ list_for_each(                                             \
                      /* list_entry_t*    IN     */   from,                   \
                      /* list_entry_t**   IN OUT */   pentry)                 \
       for (*((list_entry_t**)(pentry)) = (from)->next;                       \
            *((list_entry_t**)(pentry))!= (from);                             \
            *((list_entry_t**)(pentry)) = (*((list_entry_t**)(pentry)))->next)

/*@@list_for_next
 *
 * Loops for next list entry
 *
 * C/C++ Syntax:   
 * list_for_next(                                             
 *    list_entry_t*    IN       from,    
 *    list_entry_t**   IN OUT   pentry) 
 * {
 *    ...
 * }
 *
 * Parameters:     from           pointer to list head
 *                 entry          pointer to a list entry pointer (iterator)
 *
 * Return:         none
 *
 */
#define /* none */ list_for_next(                                             \
                      /* list_entry_t*    IN     */   from,                   \
                      /* list_entry_t**   IN OUT */   pentry)                 \
       for (*((list_entry_t**)(pentry)) = (*((list_entry_t**)(pentry)))->next;\
            *((list_entry_t**)(pentry))!= (from);                             \
            *((list_entry_t**)(pentry)) = (*((list_entry_t**)(pentry)))->next)

/*@@list_is_linked
 *
 * Is an entry linked to given list
 *
 * C/C++ Syntax:   
 * void 
 *    list_is_linked(                                            
 *       list_entry_t*    IN       head,                   
 *       list_entry_t**   IN OUT   pentry);
 *
 * Parameters:     head           pointer to the head of a list entry
 *                 entry          pointer to an entry (NULL if not linked)
 *
 * Return:         none
 *
 */
#define /* void */ list_is_linked(                                            \
                      /* list_entry_t*    IN     */   head,                   \
                      /* list_entry_t**   IN OUT */   pentry)                 \
       {                                                                      \
          list_entry_t *_i;                                                   \
          list_entry_t *_entry = *((list_entry_t **)(pentry));                \
          *((list_entry_t **) (pentry)) = NULL;                               \
          list_for_each ((head), &_i) {                                       \
             if (_i != _entry)                                                \
                continue;                                                     \
             *((void **) (pentry)) = (void*) _entry;                          \
             break;                                                           \
          }                                                                   \
       }

/*@@list_insert_head
 *
 * Inserts a list entry to the list's head
 *
 * C/C++ Syntax:   
 * void 
 *    list_insert_head(                                            
 *       list_entry_t*   IN       to,                   
 *       list_entry_t*   IN OUT   entry);
 *
 * Parameters:     to             pointer to the list's head
 *                 entry          pointer to an entry to be inserted
 *
 * Return:         none
 *
 */
#define /* void */ list_insert_head(                                          \
                      /* list_entry_t*   IN OUT */   to,                      \
                      /* list_entry_t*   IN OUT */   entry)                   \
       {                                                                      \
          list_entry_t *_head = (to);                                         \
          list_entry_t *_next = (to)->next;                                   \
          (entry)->next       = _next;                                        \
          (entry)->prev       = _head;                                        \
          _head->next         = (entry);                                      \
          _next->prev         = (entry);                                      \
       }

/*@@list_insert_tail
 *
 * Inserts a list entry to the list's tail
 *
 * C/C++ Syntax:   
 * void 
 *    list_insert_tail(                                            
 *       list_entry_t*   IN       to,                   
 *       list_entry_t*   IN OUT   entry);
 *
 * Parameters:     to             pointer to the list's head
 *                 entry          pointer to an entry to be inserted
 *
 * Return:         none
 *
 */
#define /* void */ list_insert_tail(                                          \
                      /* list_entry_t*   IN OUT */   to,                      \
                      /* list_entry_t*   IN OUT */   entry)                   \
       {                                                                      \
          list_entry_t *_head = (to);                                         \
          list_entry_t *_prev = (to)->prev;                                   \
          (entry)->next       = _head;                                        \
          (entry)->prev       = _prev;                                        \
          _prev->next         = (entry);                                      \
          _head->prev         = (entry);                                      \
       }

/*@@list_insert_after
 *
 * Inserts one list entry after another
 *
 * C/C++ Syntax:   
 * void 
 *    list_insert_after(                                            
 *       list_entry_t*   IN       to,                   
 *       list_entry_t*   IN OUT   entry);
 *
 * Parameters:     to             pointer to the old entry 
 *                 entry          pointer to the new entry to be inserted
 *
 * Return:         none
 *
 */
#define /* void */ list_insert_after(                                         \
                      /* list_entry_t*   IN OUT */   to,                      \
                      /* list_entry_t*   IN OUT */   entry)                   \
       { list_insert_head((to), (entry)) }                                    

/*@@list_insert_before
 *
 * Inserts one list entry before another
 *
 * C/C++ Syntax:   
 * void 
 *    list_insert_before(                                            
 *       list_entry_t*   IN       to,                   
 *       list_entry_t*   IN OUT   entry);
 *
 * Parameters:     to             pointer to the old entry
 *                 entry          pointer to the new entry to be inserted
 *
 * Return:         none
 *
 */
#define /* void */ list_insert_before(                                        \
                      /* list_entry_t*   IN OUT */   to,                      \
                      /* list_entry_t*   IN OUT */   entry)                   \
       { list_insert_tail((to), (entry)) }                                 

/*@@list_join_head
 *
 * Joins two lists
 *
 * C/C++ Syntax:   
 * void 
 *    list_join_head(                                            
 *       list_entry_t*   IN       to,                   
 *       list_entry_t*   IN OUT   from);
 *
 * Parameters:     to             pointer to a destination list's head
 *                 from           pointer to a source list's head
 *
 * Return:         none
 *
 */
#define /* void */ list_join_head(                                            \
                      /* list_entry_t*   IN OUT */   to,                      \
                      /* list_entry_t*   IN OUT */   from)                    \
       {                                                                      \
          list_entry_t *_first = (from)->next;                                \
          if (_first != (from)) {                                             \
             list_entry_t *_last = (from)->prev;                              \
             list_entry_t *_at   = (to)->next;                                \
             _first->prev        = (to);                                      \
             (to)->next          = _first;                                    \
             _last->next         = _at;                                       \
             _at->prev           = _last;                                     \
             list_init_entry(from);                                           \
          }                                                                   \
       }

/*@@list_join_tail
 *
 * Joins two lists
 *
 * C/C++ Syntax:   
 * void 
 *    list_join_tail(                                            
 *       list_entry_t*   IN       to,                   
 *       list_entry_t*   IN OUT   from);
 *
 * Parameters:     to             pointer to a destination list's head
 *                 from           pointer to a source list's head
 *
 * Return:         none
 *
 */
#define /* void */ list_join_tail(                                            \
                      /* list_entry_t*   IN OUT */   to,                      \
                      /* list_entry_t*   IN OUT */   from)                    \
       {                                                                      \
          list_entry_t *_first = (from)->next;                                \
          if (_first != (from)) {                                             \
             list_entry_t *_last  = (from)->prev;                             \
             list_entry_t *_after = (to)->prev;                               \
             _first->prev         = _after;                                   \
             _after->next         = _first;                                   \
             _last->next          = (to);                                     \
             (to)->prev           = _last;                                    \
             list_init_entry(from);                                           \
          }                                                                   \
       }

/*@@list_remove_entry
 *
 * Removes an entry from the lists
 *
 * C/C++ Syntax:   
 * void 
 *    list_remove_entry(                                         
 *       list_entry_t*    IN OUT   entry,                  
 *       list_entry_t**   IN OUT   pentry);
 *
 * Parameters:     entry          pointer to list entry
 *                 pentry         pointer to receiver of removed entry
 *
 * Return:         none
 *
 */
#define /* void */ list_remove_entry(                                         \
                      /* list_entry_t*    IN OUT */   entry,                  \
                      /* list_entry_t**   IN OUT */   pentry)                 \
       {                                                                      \
          list_entry_t *_next = (entry)->next;                                \
          list_entry_t *_prev = (entry)->prev;                                \
          *((list_entry_t **)(pentry)) = (entry);                             \
          _prev->next = _next;                                                \
          _next->prev = _prev;                                                \
       }

/*@@list_remove_entry_simple
 *
 * Removes an entry from the list, simple version
 *
 * C/C++ Syntax:   
 * void 
 *    list_remove_entry_simple(                                  
 *       list_entry_t*    IN OUT   entry);
 *
 * Parameters:     entry          pointer to list entry
 *
 * Return:         none
 *
 */
#define /* void */ list_remove_entry_simple(                                  \
                      /* list_entry_t*   IN OUT */   entry)                   \
       {                                                                      \
          list_entry_t* _pentry;                                              \
          list_remove_entry(entry, &_pentry);                                 \
       }

/*@@list_remove_head
 *
 * Removes an entry from the list's head
 *
 * C/C++ Syntax:   
 * void 
 *    list_remove_head(                                          
 *       list_entry_t*    IN OUT   from,                   
 *       list_entry_t**   IN OUT   pentry);
 *
 * Parameters:     from           pointer to the list's head
 *                 entry          pointer to receiver of removed entry
 *
 * Return:         none
 *
 */
#define /* void */ list_remove_head(                                          \
                      /* list_entry_t*    IN OUT */   from,                   \
                      /* list_entry_t**   IN OUT */   pentry)                 \
       {                                                                      \
          list_entry_t * _from = (from);                                      \
          if (_from->next != _from) {                                         \
             list_remove_entry (_from->next, pentry);                         \
          }                                                                   \
          else                                                                \
             *(pentry) = NULL;                                                \
       }

/*@@list_remove_tail
 *
 * Removes an entry from the list's tail
 *
 * C/C++ Syntax:   
 * void 
 *    list_remove_tail(                                          
 *       list_entry_t*    IN OUT   from,                   
 *       list_entry_t**   IN OUT   pentry);
 *
 * Parameters:     from           pointer to the list's tail
 *                 entry          pointer to receiver of removed entry
 *
 * Return:         none
 *
 */
#define /* void */ list_remove_tail(                                          \
                      /* list_entry_t*    IN OUT */   from,                   \
                      /* list_entry_t**   IN OUT */   pentry)                 \
       {                                                                      \
          list_entry_t * _from = (from);                                      \
          if (_from->prev != _from) {                                         \
             list_remove_entry (_from->prev, pentry);                         \
          }                                                                   \
          else                                                                \
             *(pentry) = NULL;                                                \
       }

/*@@list_count
 *
 * Counts a number of list entries
 *
 * C/C++ Syntax:   
 * void 
 *    list_count(                                          
 *       list_entry_t*   IN    head,                       
 *       uint*           OUT   num);
 *
 * Parameters:     head           pointer to the list's head
 *                 num            pointer to a number of list entries (uint)
 *
 * Return:         none
 *
 */
#define /* void */ list_count(                                                \
                      /* list_entry_t*   IN  */   head,                       \
                      /* uint*           OUT */   num)                        \
       {                                                                      \
          list_entry_t *_i;                                                   \
          *(num) = 0;                                                         \
          list_for_each (head, &_i)                                           \
          (*(num))++;                                                         \
       }


#ifdef __cplusplus
}
#endif

#endif
