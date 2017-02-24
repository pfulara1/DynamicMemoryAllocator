#include <stdio.h>
#include <unistd.h>  /* for system call sbrk */
#include <string.h>
#include <stdint.h>
void *sbrk(intptr_t increment);

/* 
 * 
 * Struct       : MEMORY_BLOCK
 * Description  : Each block contains meta-data followed by size
 *                
 *                
 * 
 */
typedef struct header_t {
  size_t size;
  int alignFree;
  struct header_t *next;
  struct header_t *prev;  
               
}header_block;

#define BLOCK_SIZE sizeof(header_block)



int MINIMUM_SIZE=BLOCK_SIZE+16;    /* header size, 16 for byte data */

 
size_t align2(size_t size);
header_block *getblock_freelist(size_t size);
void split_block(header_block *b, size_t s);
header_block *merge(header_block *b);
int previous_power_of_two(int x);
int isPowerOfTwo (int x);

header_block *head;

/*Memory allocate function  */
void *malloc(size_t size)
{ 
  size_t total_size;
  void *block;
  header_block *header;
       if (!size)
        {
         return NULL;
        }
        size=align2(size); /* rounding the size upto the nearest power of 2 */
        
        
        if(size < 16)
        size=16;
        header =(header_block*)getblock_freelist(size);
         if (header) {
            if((int)(header->size - size) >= MINIMUM_SIZE)
            { 
              split_block(header,size);
            }
          header->alignFree= 1;
          return (char*)header+(int)BLOCK_SIZE;
  }
        total_size =BLOCK_SIZE + size;
        if((int)total_size<=MINIMUM_SIZE)
        block = sbrk(MINIMUM_SIZE);
        else
        block = sbrk(total_size); 
        
        if (block == (void*) -1) {
        return NULL;
        }
        
        header =(header_block*)block;             
        header->size = size; 
        header->alignFree = 1;
        header->next=NULL;
        header->prev=NULL;
        
       return (char*)header+(int)BLOCK_SIZE;
}

/*get the previous power of two of the given number*/
int previous_power_of_two(int x ) {
    if (x == 0) {
        return 0;
    }
    x--; 
    x |= (x >> 1);
    x |= (x >> 2);
    x |= (x >> 4);
    x |= (x >> 8);
    x |= (x >> 16);
    return x - (x >> 1);
}

/*
 *
 * Function     : split_block
 * Description  : Splits the block passed as argument and iteratively make data blocks
 *                in the power of two.
 * Parameters   : current, which holds the block that has to be split and 
 *                size that holds the size of the block.
 * Return value : None. 
 * 
 */
void split_block(header_block *b, size_t s){

header_block *new;
header_block *ptr;
size_t y;
size_t x = (b->size - s - BLOCK_SIZE);

if(isPowerOfTwo(x))
{
new=(header_block*)((char*)b+(int)s);
new->size = x;
new->next = b->next;
new->prev=b;
new->alignFree = 0;

if(new->next)
new->next->prev=new;

b->size  = s;
b->next  = new;
}

/* now split into power of 2 blocks until it satisfies the condition*/
else
{
 x= x + BLOCK_SIZE;
 
 b->size=s;
 
 ptr=b;

 y=s;

 while((int)x > MINIMUM_SIZE)
 { 
 
   int z= previous_power_of_two(x);
   header_block* new= (header_block*)((char*)ptr+(int)y);
   new->size =(size_t)z;  
   new->next = ptr->next;
   new->prev=ptr;
   new->alignFree = 0;
   if(new->next)
   new->next->prev=new;

   ptr->next  = new;
   ptr= new;

   y=new->size+BLOCK_SIZE; 

   x=x-new->size-BLOCK_SIZE;

 }
 
} 
 
}

/*Function to align the size to the power of 2*/
size_t align2(size_t size)
{
size_t power=1;
while(power < size)
power*=2;
return power;
}

/*get free block from the linked list using best fit strategy*/
header_block *getblock_freelist(size_t size)
{  
    header_block *best = NULL; 
    header_block *node = head; 
    while(node) {

        if ((node->alignFree==0) && (node->size >= size) && (best==NULL || node->size < best->size)) {
            best = node;
            if (best->size==size) { break; }
         
        }
        node = node->next;
       
      } 
   return best;
  

}


/*function for calloc*/
void *calloc(size_t nmemb, size_t size){
 void  *p;
 size_t total;
 if(nmemb==0)return NULL; 

 total=align2(nmemb*size);
 
 if(total<16)
 total=16; 
 
 p = malloc(nmemb * size);
 
 if (!p) 
 return NULL;
 
 return memset(p, 0, total-4);

}

/*function to free block*/
void free(void *ptr){
header_block *b;

if(ptr==NULL) return;

b = (header_block*)((char*)ptr -(int)BLOCK_SIZE);

/*merge with next & previous if possible*/
b=merge(b);

/*set the block free*/
b->alignFree=0;

/* add the freed block to the list*/
if(!head)
head=b;
}

/* to check if the merged size is in power of 2 */
int isPowerOfTwo (int x)
{
 while (((x % 2) == 0) && x > 1) /* While x is even and > 1 */
   x /= 2;
 return (x == 1);
}

/*To merge possible blocks if the next or previous block is free */

header_block *merge(header_block *b){
/*merge next*/
if(b->next && b->next->alignFree==0){
int x=(int)(b->size+b->next->size);
if(isPowerOfTwo(x)){
b->size +=b->next->size;
b->next = b->next->next;
if(b->next)
b->next->prev=b;
}
}
/*merge prev*/
if(b->prev && b->prev->alignFree==0){
int y=(int)(b->size+b->prev->size);
if(isPowerOfTwo(y)){
b->size +=b->prev->size;
b->prev = b->prev->prev;
if(b->prev)
b->prev->next=b;
}
}
return b;
}

/*function to realloc*/
void *realloc(void*ptr, size_t size){
size_t newsize;
header_block *b;
void *newptr;
size_t oldsize;

if(!ptr)
return malloc(size);


if(size==0)
{
  free(ptr);
  return NULL;
}
b=(header_block*)((char*)ptr-(int)BLOCK_SIZE);
 
/*Align the size required in the power of 2 */
size=align2(size);
if(size<16)
size=16;

newsize=size+BLOCK_SIZE;


oldsize=b->size+BLOCK_SIZE;

/*check if the existing block can accomodate the request */
if(oldsize >= newsize)
return ptr; 
else
{
newptr = malloc(size);
memcpy(newptr, ptr, oldsize - BLOCK_SIZE);
free(ptr);
return newptr;
}
}





