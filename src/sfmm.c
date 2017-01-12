#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include "sfmm.h"

#define PAGE_SIZE 4096
#define QUADWORD_SIZE 8

//EASE OF USE MACROS
#define PAYLOAD(ptr) 						(void*)ptr+QUADWORD_SIZE
#define create_free_block( loc, size, prev, next) 	create_block(loc, size, 0, 0, prev, next)
#define create_allocated_block( loc, size, pad)		create_block(loc, size, 1, pad, (void*)-1, (void*)-1)
#define fail_no_mem()						errno= ENOMEM; return NULL;
#define INTERNAL_FOR_BLOCK(padding)				padding + QUADWORD_SIZE*2


//REMOVE BLOCK FROM FREELIST
void* decouple(sf_free_header* loc);

//CREATE A BLOCK OF THE CORRECT SIZE
void* create_block(sf_free_header* loc, size_t size, int alloc, int padding, sf_free_header* prev, sf_free_header* next);

//COALESCE THE BLOCK
void* coalesce(sf_free_header* block);


/**
 * All functions you make for the assignment must be implemented in this file.
 * Do not submit your assignment with a main function in this file.
 * If you submit with a main function in this file, you will get a zero.
 */

sf_free_header* freelist_head = NULL;
static void *LOWER_BOUND=0, *UPPER_BOUND=0;
static size_t i_internal=0, i_external=0, i_allocations=0, i_frees=0, i_coalesce=0;

void* decouple(sf_free_header* loc){//if this is not the freelist_head then make the previous block point to the next
	if(loc == freelist_head){
		freelist_head = loc->next;
	}else{
		(loc->prev)->next = loc->next;
		if(loc->next != NULL)	(loc->next)->prev = loc->prev;
	}return loc;
}



//THIS IS A PAIN TO DO WITHOUT MACROS
//THE MACROS WILL ENTER THE CORRECT INFO FOR EITHER FREE OR ALLOCATED BLOCKS
void* create_block(sf_free_header* loc, size_t size, int alloc, int padding, sf_free_header* prev, sf_free_header* next){
	loc->header.block_size = size>>4;
	loc->header.padding_size = padding;
	loc->header.alloc = alloc;
	if(!(next == (void*)-1 || prev == (void*)-1)){
		loc->next = next;
		loc->prev = prev;
		if(prev != NULL)//adjust previous to point at new free block
			prev->next = loc;
		if(next != NULL)//adjust next to point at new free block
			next->prev = loc;
	}
	sf_footer* footer_loc = (void*)loc + size - QUADWORD_SIZE;
	footer_loc->block_size = size>>4;
	footer_loc->alloc = alloc;
	if(alloc == 1)	 i_internal+=INTERNAL_FOR_BLOCK(padding);
	return loc;										/**//**/
}


void* coalesce(sf_free_header* block){
	
	void* start_addr = block;
	void* end_addr = start_addr + (block->header.block_size<<4);
	bool resize = false;
	sf_footer* lower_footer = start_addr-8;

	//check lower and higher blocks to see if we can coalesce
	if((void*)lower_footer > LOWER_BOUND){//LOWER BLOCK
		sf_free_header* lower_header = start_addr - (lower_footer->block_size<<4);
		if(lower_header->header.alloc == 0)
		{//its a free block!
			decouple(lower_header);
			start_addr = lower_header;
			resize = true;
		}
	}
	sf_free_header* higher_header = end_addr;
	if((void*)higher_header < UPPER_BOUND){//HIGHER BLOCK
		sf_footer* higher_footer = end_addr + (higher_header->header.block_size<<4) - 8;
		if(higher_header->header.alloc == 0)
		{//its a free block!
			decouple(higher_header);
			end_addr = ((void*)higher_footer) + 8;
			resize = true;
		}
	}
	if(resize == true){//if anything was free then we can resize our block
		//i_internal -= INTERNAL_FOR_BLOCK(0);
		create_free_block( start_addr, end_addr - start_addr, NULL, freelist_head);			/**/i_coalesce++;/**/
	}
	return start_addr;
}


void *sf_malloc(size_t size){
	if(size == 0) {
		errno = EINVAL; 
		return NULL;
	}

	if(LOWER_BOUND == 0) 
		UPPER_BOUND = LOWER_BOUND = sf_sbrk(0);
	
	if(size > PAGE_SIZE*4) {
		fail_no_mem();
	}

	size_t padding = (16 - size%16)%16;

	uint64_t real_size = QUADWORD_SIZE + padding+size + QUADWORD_SIZE;/*header + payload/padding + footer*/
	
	sf_free_header * head = freelist_head;

	while(head!=NULL){//iterate through the freelist and find a big enough free space
push_on:
		if((head->header.block_size<<4) >= real_size){				
			if(((head->header.block_size<<4) - real_size) < 32)
				real_size = head->header.block_size<<4;

			sf_free_header* next = head->next;//save the next
			uint64_t free_size = ((head->header.block_size<<4) - real_size);//size of free	
			sf_free_header* new_free_header = (void*)(((uint64_t)head)+real_size);//offset

			if(free_size > 0)//extract from the list
				if(head == freelist_head)
					freelist_head = create_free_block(new_free_header,free_size, NULL, next);//just update dont change
				else
					decouple(head), freelist_head = create_free_block(new_free_header,free_size, NULL, freelist_head);
			else//or adjust freelist_head
				if(next == NULL)
					freelist_head = NULL;
				else
					freelist_head = next, freelist_head->prev = NULL;

			return /**/i_allocations++/**/, PAYLOAD(create_allocated_block((void*) head, real_size, padding));	/**//**/
		}else//it can't fit
			if(head->next == NULL) 	
				break;			//if no more things to traverse, break the loop
			else 	
				head = head->next;
	}

	if(sf_sbrk(1) == (void*)(-1)){ //out of memory, fail
		fail_no_mem();
	}else{
		//add a page and keep going
		head = create_free_block(UPPER_BOUND, PAGE_SIZE, NULL, freelist_head);	
		UPPER_BOUND+=PAGE_SIZE;							
		freelist_head = coalesce(UPPER_BOUND-PAGE_SIZE);
		head = freelist_head;
		goto push_on;
	}
	return NULL;
}


void sf_free(void *ptr){
	if(ptr == NULL) 
		return;

	if((ptr-8 >= LOWER_BOUND && ptr+24 <= UPPER_BOUND) && (((uint64_t) ptr) % 16 == 0)){
		sf_free_header* newfree = ptr-8;
		
		//if allocated return
		if(newfree->header.alloc == 0) 
			return;	
		/**/i_internal-=INTERNAL_FOR_BLOCK(newfree->header.padding_size);i_frees++;/**/
		create_free_block(newfree, newfree->header.block_size<<4, NULL, freelist_head);
		freelist_head = coalesce(newfree);
	}else{
		errno = EFAULT;
	}
}


void *sf_realloc(void *ptr, size_t size){
	if(size == 0 || ptr == NULL)
		goto borked;
	if(!(ptr-8 >= LOWER_BOUND && ptr+24 <= UPPER_BOUND) || (((uint64_t) ptr) % 16 != 0)){
		errno = EFAULT;
		return NULL;
	}
	sf_header* curr_h = (ptr-8);
	size_t padding = (16 - size%16)%16;
	size_t new_block_size = 8 + size + padding + 8;

	if((curr_h)->alloc == 1){//if the block is allocated
		size_t cur_size = curr_h->block_size<<4;
		size_t old_padding = curr_h->padding_size;
		sf_free_header * next = ((void*)curr_h) + cur_size;

		if(cur_size > new_block_size)//less than current block
		{
			size_t diff = cur_size - new_block_size;
			if(diff < 32) {
				//splinter - update padding 
				if((void*)next < UPPER_BOUND && next->header.alloc == 0){//merge
					create_allocated_block((void*)curr_h, new_block_size, padding), decouple(next);
					freelist_head = create_free_block(((void*)curr_h) + new_block_size, diff + (next->header.block_size<<4), NULL, freelist_head);
					return /**/i_internal -= INTERNAL_FOR_BLOCK(old_padding), i_allocations++,i_frees++,/**/ ptr;/**//**/
				}else{ 
					//splinter
					curr_h->padding_size = padding;
					return /**/i_internal += padding - curr_h->padding_size/**/, ptr;
				}
			}else{ 
				// not a splinter - gotta split the block
				create_allocated_block((void*)curr_h, new_block_size, padding);
				freelist_head = coalesce(create_free_block(((void*)curr_h) + new_block_size, diff, NULL, freelist_head));
				return /**/i_internal -= INTERNAL_FOR_BLOCK(old_padding), i_allocations++,i_frees++,/**/ ptr;	/**//**/
			}
		}
		else if(cur_size == new_block_size)//same size, just fix padding
		{
			curr_h->padding_size = padding;
			return /**/i_internal += padding - curr_h->padding_size/**/, ptr;			/**//**/
		}
		else //new_block_size > cur_size
		{
			if((void*)next < UPPER_BOUND && next->header.alloc == 0) //next block is free
			{
				size_t next_size = next->header.block_size<<4;
				size_t total_block = cur_size + next_size;
				if(total_block >= new_block_size)//enough space
				{
					if(total_block == new_block_size || total_block == new_block_size+16) {//same size or splinter
						decouple(next);
					     	create_allocated_block((void*)curr_h, total_block, padding);
						return /**/i_internal -= INTERNAL_FOR_BLOCK(old_padding), i_allocations++,/**/ptr;/**//**/	
					}else if(total_block > new_block_size){//bigger block
						size_t diff = total_block-new_block_size;
						decouple(next);
					     	create_allocated_block((void*)curr_h, new_block_size, padding);
						freelist_head = create_free_block(((void*)curr_h)+new_block_size, diff, NULL, freelist_head);
						return /**/i_internal -= INTERNAL_FOR_BLOCK(old_padding), i_allocations++, i_frees++,/**/ ptr;	/**//**/
					}
				}
			}//malloc and free copy the stuff
			i_internal -= INTERNAL_FOR_BLOCK(old_padding);
			void* ptr2 = sf_malloc(size);
			if(ptr2 == NULL)
				goto borked;
			memcpy(ptr2, ptr, size);
			sf_free(ptr);
			return ptr2;
		}
	}else	{//if the block is free
borked:
		errno = EINVAL;
		return NULL;
	}
}


int sf_info(info* meminfo){
	if(meminfo == NULL) return -1;
	i_external = 0;
	for(sf_free_header *freeb = freelist_head; freeb!=NULL; freeb=freeb->next)
	{i_external += (freeb->header.block_size<<4);}
	meminfo->internal = i_internal;
	meminfo->external = i_external;
	meminfo->allocations = i_allocations;
	meminfo->frees = i_frees;
	meminfo->coalesce = i_coalesce;
	return 0;
}
