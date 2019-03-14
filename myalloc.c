#include <stdio.h>
#include <stdlib.h>
#include "myalloc.h"

/* change me to 1 for more debugging information
 * change me to 0 for time testing and to clear your mind
 */
//	Rudolf Musika. 	&& Alfonzo Sainz	//
#define DEBUG 0

void *__heap = NULL;
node_t *__head = NULL;

header_t *get_header(void *ptr) {
  return (header_t *) (ptr - sizeof(header_t));
}

node_t *get_node(void *ptr){
	return (node_t *) (ptr - sizeof(node_t));
}

void print_header(header_t *header) {
  printf("[header_t @ %p | buffer @ %p size: %lu magic: %08lx]\n",
         header,
         ((void *) header + sizeof(header_t)),
         header->size,
         header->magic);
}

void print_node(node_t *node) {
  printf("[node @ %p | free region @ %p size: %lu next: %p]\n",
         node,
         ((void *) node + sizeof(node_t)),
         node->size,
         node->next);
}

void print_freelist_from(node_t *node) {
  printf("\nPrinting freelist from %p\n", node);
  while (node != NULL) {
    print_node(node);
    node = node->next;
  }
}

node_t* sort_linked_list(node_t *old_head){
	// Include recursive base case
	if(old_head == NULL || old_head->next == NULL){
		return old_head;
	}
	// Find smallest node
	node_t *curr;
	node_t *smallest;
	node_t *smallestPrev;
	node_t *prev;
	curr = old_head;
	smallest = old_head;
	prev = old_head;
	smallestPrev = old_head;

	while(curr != NULL) {
		if((void*) curr < (void*) smallest) {
			smallestPrev = prev;
			smallest = curr;
		}
		prev = curr;
		curr = curr->next;
	}

	// Switching first node and smallest node
	node_t* tmp;
	if(smallest != old_head){
		smallestPrev->next = old_head;
		tmp = old_head->next;
		old_head->next = smallest->next;
		smallest->next = tmp;
	}

	// Recurse
	smallest->next = sort_linked_list(smallest->next);
	return smallest;
}

void coalesce_freelist() {
  /* coalesce all neighboring free regions in the free list */

  if (DEBUG) printf("In coalesce freelist...\n");
  __head = sort_linked_list(__head);
  node_t *target = __head;
  node_t *node = target->next;
  node_t *prev = target;
  long unsigned int coalescedVersion = 0;

	while(target != NULL && node != NULL){
		if((void*) node == (void*) target + target->size + sizeof(header_t)){
			printf("Found coalescible\n");
			node_t *new_next = node->next;
			target->size = target->size + node->size + sizeof(header_t);
			target->next = new_next;
			coalescedVersion =1;
		}else{
			printf("Found NOT coalescible: ");
			printf("%08lx != %08lx\n", ((void *) node), ((void *) target + target->size + sizeof(header_t)));
		}
		prev = target;
		target = target->next;
		node = target->next;
	}

  /* traverse the free list, coalescing neighboring regions!
   * some hints:
   * --> it might be easier if you sort the free list first!
   * --> it might require multiple passes over the free list!
   * --> it might be easier if you call some helper functions from here
   * --> see print_free_list_from for basic code for traversing a
   *     linked list!
   */
	if(coalescedVersion ==1){ coalesce_freelist();}
}

void destroy_heap() {
  /* after calling this the heap and free list will be wiped
   * and you can make new allocations and frees on a "blank slate"
   */
  free(__heap);
  __heap = NULL;
  __head = NULL;
}

/* In reality, the kernel or memory allocator sets up the initial heap. But in
 * our memory allocator, we have to allocate our heap manually, using malloc().
 * YOU MUST NOT ADD MALLOC CALLS TO YOUR FINAL PROGRAM!
 */
void init_heap() {
  /* FOR OFFICE USE ONLY */

  if ((__heap = malloc(HEAPSIZE)) == NULL) {
    printf("Couldn't initialize heap!\n");
    exit(1);
  }

  __head = (node_t *) __heap;
  __head->size = HEAPSIZE - sizeof(header_t);
  __head->next = NULL;

  if (DEBUG) printf("heap: %p\n", __heap);
  if (DEBUG) print_node(__head);

}

void *first_fit(size_t req_size) {
  void *ptr = __head;/* pointer to the match that we'll return */
  long unsigned int found_space_youget = 0; 

  if (DEBUG)
    printf("In first_fit with size: %u and freelist @ %p\n",
           (unsigned) req_size, __head);

  node_t *listitem = __head; /* cursor into our linked list */
  node_t *prev = NULL; /* if listitem is __head, then prev must be null */
  header_t *alloc; /* a pointer to a header you can use for your allocation */

// while the listitem is NOT Null,do the following.
  //fprintf(2, "Doing first fit");
  while(listitem != NULL){
    if((req_size + sizeof(header_t)) <= listitem->size && req_size > 0){
// Initialize helpful working variables you plan to use.
	long unsigned int listitem_is_head = 0;
	node_t *new_freelist_item_header = NULL;

// Get a copy of the listitem size to be used.
	long unsigned int listitem_size = listitem->size;
	node_t *listitem_next = listitem->next;
// The alloc header replaces the listitem
      alloc = (void*) listitem;
      alloc->magic = HEAPMAGIC;
      alloc->size = req_size;
// The ptr is returned and points to the actual free space we allocate
      ptr = (void*) alloc + sizeof(header_t);

// Lets find out if the __head is pointing to this listitem. If so, then this listitem 
//is the first in the freelist
	if(__head == listitem){
		listitem_is_head = 1;
      }

// We only want to create a new freelist item if the remaining space is more than
// enough for a header and a single byte of memory.
      /*listitem->size = listitem->size - (req_size + sizeof(header_t));//Taken Out*/
     	if((listitem_size - req_size) >= (sizeof(header_t))){
// Create a new freelist header at the start of the allocation plus the size of the allocation (the end of the allocation)
		new_freelist_item_header = (void*) ptr + req_size;
		new_freelist_item_header->size = listitem_size - req_size - sizeof(header_t);
		new_freelist_item_header->next = listitem_next;
	}

// If the listitem is the head, then we need to update head
	if(listitem_is_head == 1){
		printf("=============> List item is head\n");
// If we were able to fit a new header into the remaining free space
	if(new_freelist_item_header != NULL){
		printf("=============> Setting head to the new freelist node:\n");
		__head = new_freelist_item_header;
	}else{
// Otherwise we'll need to set the head to the next freelist item
		printf("=============> Setting head to the next freelist node\n");
		__head = listitem_next;
	}
	}else{
		printf("=============> List item is not head\n");
// The listitem is not the __head, so we need to update the previous free region to whichever the next freelist item header is
// If we were able to fit a new header into the remaining free space
	if(new_freelist_item_header != NULL){
		printf("=============> Setting prev's next to the new freelist node\n");
		prev->next = new_freelist_item_header;
	}else{
// Otherwise we'll need to set the prev to the next freelist item
		printf("=============> Setting prev's next to the next freelist node\n");
		prev->next = listitem_next;
	}
	}

	printf("=============> Allocation over. Printing freelist:\n");
	print_freelist_from(__head);
	found_space_youget = 1;
	break;
	}	
	
	if(listitem->size > 0){
        	prev = listitem;
	}
    
    listitem = listitem->next;	// Insert listitem here.
  }
  /*
  if(listitem == NULL)
    return NULL;
  */	
  /* traverse the free list from __head! when you encounter a region that
   * is large enough to hold the buffer and required header, use it!
   * If the region is larger than you need, split the buffer into two
   * regions: first, the region that you allocate and second, a new (smaller)
   * free region that goes on the free list in the same spot as the old free
   * list node_t.
   *
   * If you traverse the whole list and can't find space, return a null
   * pointer! :(
   *
   * Hints:
   * --> see print_freelist_from to see how to traverse a linked list
   * --> remember to keep track of the previous free region (prev) so
   *     that, when you divide a free region, you can splice the linked
   *     list together (you'll either use an entire free region, so you
   *     point prev to what used to be next, or you'll create a new
   *     (smaller) free region, which should have the same prev and the next
   *     of the old region.
   * --> If you divide a region, remember to update prev's next pointer!
   */

  if(found_space_youget == 0){
	ptr = NULL;
  }

  if (DEBUG) printf("Returning pointer: %p\n", ptr);
  return ptr;

}

/* myalloc returns a void pointer to size bytes or NULL if it can't.
 * myalloc will check the free regions in the free list, which is pointed to by
 * the pointer __head.
 */

void *myalloc(size_t size) {
  if (DEBUG) printf("\nIn myalloc:\n");
  void *ptr = NULL;

  /* initialize the heap if it hasn't been */
  if (__heap == NULL) {
    if (DEBUG) printf("*** Heap is NULL: Initializing ***\n");
    init_heap();
  }

  /* perform allocation */
  /* search __head for first fit */
  if (DEBUG) printf("Going to do allocation.\n");

  ptr = first_fit(size); /* all the work really happens in first_fit */

  if (DEBUG) printf("__head is now @ %p\n", __head);

  return ptr;

}

/* myfree takes in a pointer _that was allocated by myfree_ and deallocates it,
 * returning it to the free list (__head) like free(), myfree() returns
 * nothing.  If a user tries to myfree() a buffer that was already freed, was
 * allocated by malloc(), or basically any other use, the behavior is
 * undefined.
 */
void myfree(void *ptr) {

  if (DEBUG) {printf("\nIn myfree with pointer %p\n", ptr);}

  header_t *header = get_header(ptr); /* get the start of a header from a pointer */

  if (DEBUG) { print_header(header); }

  if (header->magic != HEAPMAGIC) {
    printf("Header is missing its magic number!!\n");
    printf("It should be '%08lx'\n", HEAPMAGIC);
    printf("But it is '%08lx'\n", header->magic);
    printf("The heap is corrupt!\n");
    return;
  }


	printf("header size: %d\n", header->size);
	printf("Before free: \n");
	print_freelist_from(__head);

  /* free the buffer pointed to by ptr!
   * To do this, save the location of the old head (hint, it's __head).
   * Then, change the allocation header_t to a node_t. Point __head
   * at the new node_t and update the new head's next to point to the
   * old head. Voila! You've just turned an allocated buffer into a
   * free region!
   */

  /* save the current __head of the freelist */
  node_t *old_head = __head;

  /* now set the __head to point to the header_t for the buffer being freed */
  __head = (node_t*) header;
  printf("header magic vs __head next: %08lx = %08lx\n", header->magic, __head->next);
  printf("__head size: %d\n", __head->size);

  /* set the new head's next to point to the old head that you saved */
  __head->next = old_head;

  /* PROFIT!!! */
	printf("After free: \n");
	print_freelist_from(__head);
}