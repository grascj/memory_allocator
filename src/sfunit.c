#include <criterion/criterion.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include "sfmm.h"

/**
 *  HERE ARE OUR TEST CASES NOT ALL SHOULD BE GIVEN STUDENTS
 *  REMINDER MAX ALLOCATIONS MAY NOT EXCEED 4 * 4096 or 16384 or 128KB
 */
Test(sf_memsuite, Malloc_an_Integer, .init = sf_mem_init, .fini = sf_mem_fini) {
    int *x = sf_malloc(sizeof(int));
    *x = 4;
    cr_assert(*x == 4, "Failed to properly sf_malloc space for an integer!");
}

Test(sf_memsuite, Free_block_check_header_footer_values, .init = sf_mem_init, .fini = sf_mem_fini) {
    void *pointer = sf_malloc(sizeof(short));
    sf_free(pointer);
    pointer = pointer - 8;
    sf_header *sfHeader = (sf_header *) pointer;
    cr_assert(sfHeader->alloc == 0, "Alloc bit in header is not 0!\n");
    sf_footer *sfFooter = (sf_footer *) (pointer - 8 + (sfHeader->block_size << 4));
    cr_assert(sfFooter->alloc == 0, "Alloc bit in the footer is not 0!\n");
}

Test(sf_memsuite, PaddingSize_Check_char, .init = sf_mem_init, .fini = sf_mem_fini) {
    void *pointer = sf_malloc(sizeof(char));
    pointer = pointer - 8;
}

Test(sf_memsuite, Check_next_prev_pointers_of_free_block_at_head_of_list, .init = sf_mem_init, .fini = sf_mem_fini) {
    int *x = sf_malloc(4);
    memset(x, 0, 4);
    cr_assert(freelist_head->next == NULL);
    cr_assert(freelist_head->prev == NULL);
}


Test(sf_memsuite, Coalesce_no_coalescing, .init = sf_mem_init, .fini = sf_mem_fini) {
	void *x = sf_malloc(4);
	void *y = sf_malloc(4);
	memset(y, 0xFF, 4);
	sf_free(x);
	cr_assert(freelist_head == x-8);
	sf_free_header *headofx = (sf_free_header*) (x-8);
	sf_footer *footofx = (sf_footer*) (x - 8 + (headofx->header.block_size << 4)) - 8;

	//All of the below should be true if there was no coalescing
	cr_assert(headofx->header.alloc == 0);
	cr_assert(headofx->header.block_size << 4 == 32);

	cr_assert(footofx->alloc == 0);
	cr_assert(footofx->block_size << 4 == 32);




}






//############################################
// STUDENT UNIT TESTS SHOULD BE WRITTEN BELOW
// DO NOT DELETE THESE COMMENTS
//############################################

Test(sf_memsuite, free_in_order, .init = sf_mem_init, .fini = sf_mem_fini) {//1
	//allocating some things and then freeing sequentially in order
	sf_malloc(4);
	void *a = sf_malloc(4);
	void *b = sf_malloc(4);
	void *c = sf_malloc(4);
	sf_malloc(4);

	sf_free(a);
	sf_free(b);
	sf_free(c);

	cr_assert(freelist_head == a-8);
	cr_assert(freelist_head->header.alloc == 0);
	cr_assert(freelist_head->header.block_size << 4 == 96,"Expected 96, was given: %d.",freelist_head->header.block_size<<4);

	sf_footer* freelist_foot = (void*)freelist_head + (freelist_head->header.block_size<<4) -8;
	cr_assert(freelist_foot->alloc == 0);
	cr_assert(freelist_foot->block_size << 4 == 96,"Expected 96, was given: %d.",freelist_head->header.block_size<<4);

	cr_assert(freelist_head->next != NULL);
	cr_assert(freelist_head->prev == NULL);

	//check to see if the explicit list is intact
	sf_free_header* next = freelist_head->next;
	cr_assert(next->header.alloc == 0);
	cr_assert(next->header.block_size << 4 == 3936,"Expected 3936, was given: %d.",next->header.block_size<<4);

	cr_assert(next->next == NULL);
	cr_assert(next->prev == freelist_head);

	sf_footer* next_foot = (void*)next + (next->header.block_size<<4) -8;
	cr_assert(next_foot->alloc == 0);
	cr_assert(next_foot->block_size << 4 == 3936,"Expected 3936, was given: %d.",next_foot->block_size<<4);
}

//2
Test(sf_memsuite, free_in_reverse, .init = sf_mem_init, .fini = sf_mem_fini) {
	//allocating some things and then freeing in reverse
	//make sure coalesce works
	sf_malloc(4);
	void *a = sf_malloc(4);
	void *b = sf_malloc(4);
	void *c = sf_malloc(4);
	sf_malloc(4);

	sf_free(c);
	sf_free(b);
	sf_free(a);

	cr_assert(freelist_head == a-8);
	cr_assert(freelist_head->header.alloc == 0);
	cr_assert(freelist_head->header.block_size << 4 == 96,"Expected 96, was given: %d.",freelist_head->header.block_size<<4);

	sf_footer* freelist_foot = (void*)freelist_head + (freelist_head->header.block_size<<4) -8;
	cr_assert(freelist_foot->alloc == 0);
	cr_assert(freelist_foot->block_size << 4 == 96,"Expected 96, was given: %d.",freelist_head->header.block_size<<4);

	cr_assert(freelist_head->next != NULL);
	cr_assert(freelist_head->prev == NULL);

	//check to see if the explicit list is intact
	sf_free_header* next = freelist_head->next;
	cr_assert(next->header.alloc == 0);
	cr_assert(next->header.block_size << 4 == 3936,"Expected 3936, was given: %d.",next->header.block_size<<4);

	cr_assert(next->next == NULL);
	cr_assert(next->prev == freelist_head);

	sf_footer* next_foot = (void*)next + (next->header.block_size<<4) -8;
	cr_assert(next_foot->alloc == 0);
	cr_assert(next_foot->block_size << 4 == 3936,"Expected 3936, was given: %d.",next_foot->block_size<<4);
}

//3
Test(sf_memsuite, free_middle_last, .init = sf_mem_init, .fini = sf_mem_fini) {
	//freeing in an odd order
	sf_malloc(4);
	void *a = sf_malloc(4);
	void *b = sf_malloc(4);
	void *c = sf_malloc(4);
	sf_malloc(4);

	sf_free(a);
	sf_free(c);
	sf_free(b);

	cr_assert(freelist_head == a-8);
	cr_assert(freelist_head->header.alloc == 0);
	cr_assert(freelist_head->header.block_size << 4 == 96,"Expected 96, was given: %d.",freelist_head->header.block_size<<4);

	sf_footer* freelist_foot = (void*)freelist_head + (freelist_head->header.block_size<<4) -8;
	cr_assert(freelist_foot->alloc == 0);
	cr_assert(freelist_foot->block_size << 4 == 96,"Expected 96, was given: %d.",freelist_foot->block_size<<4);

	cr_assert(freelist_head->next != NULL);
	cr_assert(freelist_head->prev == NULL);

	//check to see if the explicit list is intact
	sf_free_header* next = freelist_head->next;
	cr_assert(next->header.alloc == 0);
	cr_assert(next->header.block_size << 4 == 3936,"Expected 3936, was given: %d.",next->header.block_size<<4);

	cr_assert(next->next == NULL);
	cr_assert(next->prev == freelist_head);

	sf_footer* next_foot = (void*)next + (next->header.block_size<<4) -8;
	cr_assert(next_foot->alloc == 0);
	cr_assert(next_foot->block_size << 4 == 3936,"Expected 3936, was given: %d.",next_foot->block_size<<4);
}


//3.5
Test(sf_memsuite, free_middle_last_part_two, .init = sf_mem_init, .fini = sf_mem_fini) {
	//freeing in an odd order
	sf_malloc(4);
	void *a = sf_malloc(4);
	void *b = sf_malloc(4);
	void *c = sf_malloc(4);
	sf_malloc(4);

	sf_free(c);
	sf_free(a);
	sf_free(b);

	cr_assert(freelist_head == a-8);
	cr_assert(freelist_head->header.alloc == 0);
	cr_assert(freelist_head->header.block_size << 4 == 96,"Expected 96, was given: %d.",freelist_head->header.block_size<<4);

	sf_footer* freelist_foot = (void*)freelist_head + (freelist_head->header.block_size<<4) -8;
	cr_assert(freelist_foot->alloc == 0);
	cr_assert(freelist_foot->block_size << 4 == 96,"Expected 96, was given: %d.",freelist_foot->block_size<<4);

	cr_assert(freelist_head->next != NULL);
	cr_assert(freelist_head->prev == NULL);

	//check to see if the explicit list is intact
	sf_free_header* next = freelist_head->next;
	cr_assert(next->header.alloc == 0);
	cr_assert(next->header.block_size << 4 == 3936,"Expected 3936, was given: %d.",next->header.block_size<<4);

	cr_assert(next->next == NULL);
	cr_assert(next->prev == freelist_head);

	sf_footer* next_foot = (void*)next + (next->header.block_size<<4) -8;
	cr_assert(next_foot->alloc == 0);
	cr_assert(next_foot->block_size << 4 == 3936,"Expected 3936, was given: %d.",next_foot->block_size<<4);
}

//4
Test(sf_memsuite, free_whole_block, .init = sf_mem_init, .fini = sf_mem_fini) {
	void *a = sf_malloc(4);
	void *b = sf_malloc(4);
	void *c = sf_malloc(5000);

	sf_free(a);
	sf_free(c);
	sf_free(b);

	cr_assert(freelist_head == a-8);
	cr_assert(freelist_head->header.alloc == 0);
	cr_assert(freelist_head->header.block_size << 4 == 8192,"Expected 8192, was given: %d.",freelist_head->header.block_size<<4);

	sf_footer* freelist_foot = (void*)freelist_head + (freelist_head->header.block_size<<4) -8;
	cr_assert(freelist_foot->alloc == 0);
	cr_assert(freelist_foot->block_size << 4 == 8192,"Expected 8192, was given: %d.",freelist_foot->block_size<<4);

	cr_assert(freelist_head->next == NULL);
	cr_assert(freelist_head->prev == NULL);
}



//5
Test(sf_memsuite, malloc_then_free_all_pages, .init = sf_mem_init, .fini = sf_mem_fini) {
	//seeing if this breaks malloc correctly
	void* a = sf_malloc(16337);
	
	void* nullpls = sf_malloc(32);
	cr_assert(nullpls == NULL);
	cr_assert(errno == ENOMEM);
	errno = 0;

	sf_free_header *ahead = a-8;
	cr_assert(ahead->header.alloc == 1);
	cr_assert(ahead->header.block_size << 4 == 16384,"Expected 16384, was given: %d.",ahead->header.block_size<<4);

	sf_free(a);
	cr_assert(freelist_head->next == NULL);
	cr_assert(freelist_head->prev == NULL);

 	sf_malloc(4);
	void *b = sf_malloc(4);
	void *c = sf_malloc(4);
	sf_free(b);

	cr_assert((sf_free_header*)freelist_head->next == c-8+32);
}



//6
Test(sf_memsuite, please_refrain_from_adding_more_pages, .init = sf_mem_init, .fini = sf_mem_fini) {
	//seeing if it doesnt add a oage
	cr_assert(errno == 0);
	void* a = sf_malloc(4080);
	
	cr_assert(freelist_head == NULL);
	sf_free(a);
	void* b = sf_malloc(2048-16);
	void* c = sf_malloc(1024-16);
	void* d = sf_malloc(1024-16);
	cr_assert(freelist_head == NULL);
	sf_free(b);
	sf_free(d);
	sf_free(c);

	cr_assert(freelist_head->header.block_size<<4 == 4096);
	cr_assert(freelist_head->next == NULL);
	cr_assert(freelist_head->prev == NULL);
}

//7
Test(sf_memsuite, test_taking_out_of_freelist, .init = sf_mem_init, .fini = sf_mem_fini) {
	//just some more freeing, this time with a full page (NULL freelist head)
	sf_malloc(32);	
	int* a = sf_malloc(32);	
	int* b = sf_malloc(32);	
	int* c = sf_malloc(32);	
	sf_malloc(32);	
	sf_malloc(3840);
	cr_assert(freelist_head == NULL);
	
	sf_free(a);
	sf_free(c);
	sf_free(b);
	cr_assert(freelist_head == ((void*)a)-8);	
	cr_assert(freelist_head->next == NULL);	
	
}	


//8
Test(sf_memsuite, test_some_small_reallocs, .init = sf_mem_init, .fini = sf_mem_fini) {
	//testing reallocs when the size is smaller
	void *b = sf_malloc(128);
	sf_realloc(b, 10);
	cr_assert(((sf_header*)(b-8))->block_size<<4 == 32);
	cr_assert(((sf_header*)(b-8))->padding_size == 6);
	cr_assert(freelist_head->header.block_size<<4 == 4064);

	sf_realloc(b, 1);
	cr_assert(((sf_header*)(b-8))->block_size<<4 == 32);
	cr_assert(((sf_header*)(b-8))->padding_size == 15);
	cr_assert(freelist_head->header.block_size<<4 == 4064);

	sf_free(b);
	cr_assert(b-8 == freelist_head);
	cr_assert(freelist_head->header.block_size<<4 == 4096);

}

//9
Test(sf_memsuite, test_some_big_reallocs, .init = sf_mem_init, .fini = sf_mem_fini) {
	//testing reallocs when it's bigger
	sf_malloc(32);	
	int *a = sf_malloc(32);	
	int* b = sf_malloc(32);	
	sf_malloc(32);	
	int *c = 
	sf_malloc(32);	
	int* d = 
	sf_malloc(32);	
	sf_malloc(32);	
	sf_malloc(3744); //4096
	
	//splinter
	cr_assert(freelist_head == NULL);
	sf_free(b);
	sf_realloc(a,64); //80 + 16 for splinter
	cr_assert(freelist_head == NULL);
	
	
	//gotta malloc
	sf_free(c);
	sf_realloc(d,128);
	cr_assert(freelist_head == (void*)c-8);
	
	sf_malloc(80);
	sf_malloc(64);
	void* e = sf_malloc(64);
	void *f = sf_malloc(64);
	sf_malloc(64);
	sf_malloc(64);
	
	//extend the header
	sf_free(f);
	e = sf_realloc(e,90);
	
	cr_assert(freelist_head == (e-8)+112);
	cr_assert(freelist_head->header.block_size<<4 == 48);
	cr_assert(((sf_free_header *)(e-8))->header.block_size<<4 == 112);
}


//10
Test(sf_memsuite, more_realloc_tests, .init = sf_mem_init, .fini = sf_mem_fini) {
	//seeing if realloc can set everything to null right
	sf_malloc(32);	
	void *a = sf_malloc(32);	
	void* b = sf_malloc(32);	
	sf_malloc(32);	
	sf_malloc(32);	
	
	
	sf_realloc(a,256);
	cr_assert(freelist_head == a-8);
	
	sf_free(b);
	cr_assert(freelist_head == a-8);
	cr_assert(freelist_head->next->next == NULL);
	
}


//11
Test(sf_memsuite, realloc_stress_test, .init = sf_mem_init, .fini = sf_mem_fini) {
	//testing to see if the memory all lines up
	void *a = sf_malloc(32);	//48
	void* b = sf_malloc(32);	//48
	void* c = sf_malloc(32);	//48
	void *d = sf_malloc(4);			//32
					//176

	a = sf_realloc(a,48);		//wont fit, get freed, allocate 64 after the 32
					//64
	b = sf_realloc(b,48);		//wont fit, get freed, allocate 64 after the 32
					//64

	c = sf_realloc(c,4); 		//will fit in a
					//32 bytes
					
	size_t total = (((sf_free_header*)(a-8))->header.block_size<<4);
	total += (((sf_free_header*)(b-8))->header.block_size<<4);
	total += (((sf_free_header*)(c-8))->header.block_size<<4);
	total += (((sf_free_header*)(d-8))->header.block_size<<4);
	total += (freelist_head->header.block_size<<4);
	total += (freelist_head->next->header.block_size<<4);
	
	cr_assert(total == 4096);
	errno = 0;

}


//12
Test(sf_memsuite, crashing_this_malloc, .init = sf_mem_init, .fini = sf_mem_fini) {
	void* a = sf_malloc(20000);
	cr_assert(a == NULL);
	cr_assert(errno != 0);
	errno = 0;
}

//13
Test(sf_memsuite, realloc_value_test, .init = sf_mem_init, .fini = sf_mem_fini) {
	//see if i can actually use realloc to copy memory
	sf_malloc(32);	
	long double *a = sf_malloc(32);	
	*a = 100;
	long double* b = sf_malloc(32);	
	*b = 31;
	sf_malloc(32);	
	cr_assert(*a == 100);
	cr_assert(*b == 31);

	b = realloc(b,128);
	cr_assert(*b == 31);
	

	*b += *a;
	sf_free(a);

	b = realloc(b,16);
	cr_assert(*b == 31 + 100);
	errno = 0;
}



//14
Test(sf_memsuite, test_decouple, .init = sf_mem_init, .fini = sf_mem_fini) {
	//SEE IF MY LIST STAYS INTACT
	sf_malloc(4);
	sf_malloc(4);
	void* c = sf_malloc(16);
	sf_malloc(64);
	void* d = sf_malloc(128);
	sf_malloc(256);
	void* e = sf_malloc(4);
	sf_malloc(4);
	
	sf_free(c);
	sf_free(d);
	sf_free(e);

	sf_malloc(32);

//	for(sf_free_header *freeb = freelist_head; freeb!=NULL; freeb=freeb->next)
//	{ sf_blockprint(freeb);}
	errno = 0;

}


//15
Test(sf_memsuite, test_sf_info, .init = sf_mem_init, .fini = sf_mem_fini) {
	//testing sf_info returns
	printf("START\n");
	sf_malloc(4);//32
	void* a = sf_malloc(32);//48
	a = sf_realloc(a,16);//32

	info k;
	sf_info(&k);
	printf("internal:%li\n", k.internal);
	printf("external:%li\n", k.external);
	printf("allocations:%li\n", k.allocations);
	printf("frees:%li\n", k.frees);
	printf("coalesce:%li\n", k.coalesce);
	printf("END\n");
	errno = 0;
}


//16
Test(sf_memsuite, test_realloc_splinter_when_next_block_is_free, .init = sf_mem_init, .fini = sf_mem_fini) {
	//testing if the 16 bytes are merged correctly
	sf_malloc(4);
	void* a = sf_malloc(32);
	a = sf_realloc(a,16);
	cr_assert(freelist_head->header.block_size<<4 == 4032);

	//now see if it just splinters
	sf_free(a);//32
	a = sf_malloc(32);//32 + _48
	sf_malloc(4);//80 + _32
	a = sf_realloc(a, 16); //112 still _48 because splinter
	cr_assert(((sf_free_header*)(a-8))->header.block_size<<4 == 48);
	cr_assert(freelist_head->header.block_size<<4 == 4016 - 32);
	errno = 0;
}

//17
Test(sf_memsuite, test_freeing_outside_our_heap, .init = sf_mem_init, .fini = sf_mem_fini) {
	//testing to see if freeing outside of the heap fails (set errno ya dingus)
	void* a = sf_malloc(32);
	sf_free(a-8);
	cr_assert(errno != 0);
	errno = 0;
}

