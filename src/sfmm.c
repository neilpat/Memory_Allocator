/**
 * All functions you make for the assignment must be implemented in this file.
 * Do not submit your assignment with a main function in this file.
 * If you submit with a main function in this file, you will get a zero.
 */
#include "sfmm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/**
 * You should store the heads of your free lists in these variables.
 * Doing so will make it accessible via the extern statement in sfmm.h
 * which will allow you to pass the address to sf_snapshot in a different file.
 */
free_list seg_free_list[4] = {
    {NULL, LIST_1_MIN, LIST_1_MAX},
    {NULL, LIST_2_MIN, LIST_2_MAX},
    {NULL, LIST_3_MIN, LIST_3_MAX},
    {NULL, LIST_4_MIN, LIST_4_MAX}
};
int sf_errno = 0;
int page_track = 0;
// int space = 0;

void ptr_check(void *);
void *sf_malloc(size_t size) {
    int space = 0;
    if(size == 0 || size > 16384){
        sf_errno = 22;              /*ENOMEM = 12*/
        return NULL;
    }
    sf_header *header1;
    void *pay_load;
    //FINDING THE BLOCK SIZE BY ADDING 8 BYTES FOR HEADER AND FOOTER
    int size_1 = (int)size + 8 + 8;
    int padding = 0;
    if(size_1 % 16 != 0){
        size_1 = size_1 + (16 - size_1%16);
        padding = 1;
    }
    int flag = 0;
    sf_free_header *cursor1;
    //SEE IF ANY LIST HAS A BLOCK TO RETURN FROM THE REQUESTED SIZE
    here:;
    for(int i = 0; i < 4;i++){
        cursor1 = seg_free_list[i].head;
        while(flag == 0 && cursor1 != NULL){
            if(((cursor1->header).block_size) << 4 >= size_1){
                seg_free_list[i].head = seg_free_list[i].head->next;
                pay_load = (char*)cursor1 + 8;
                flag = 1;
                break;
            }else{
                cursor1 = seg_free_list[i].head->next;
            }
        }
        if(flag == 1){
            space = (cursor1->header).block_size << 4;
            break;
        }
    }
    //IF THE REQUESTED BLOCK SIZE WAS NOT FOUND IN THE LISTS
    int remaining_block = 0;
    int enough = 0;
    void *page;
    int colBlock = 0;;
    sf_header *temp_header;
    while(enough == 0 && flag == 0){
        page = sf_sbrk();
        space += 4096;
        remaining_block = space - size_1;
        page_track++;
        if(page_track>4){
            sf_errno = 12;
            return NULL;
        }else{
            temp_header = page;
                //CHECK IF PREVIOUS SPACE BLOCK IS FREE
            sf_footer *temp_footer = (sf_footer*)((char*)page - 8);                             //PROBLEM HERE
            int allocated_check = temp_footer->allocated;
                //IF BLOCK PREVIOUS TO START OF PAGE IS FREE THAN ENTER
            if(allocated_check == 0){
                temp_header = (sf_header*)((char*)page - (temp_footer->block_size << 4));
                    //START SEARCHING LISTS TO FIND THE PREVIOUS BLOCK
                for(int i = 0;i < 4;i++){
                    sf_free_header *cursor = seg_free_list[i].head;
                    if(&(cursor->header) == temp_header){
                            //SIZE_1 HAD THE TOTAL SIZE NEEDED FOR THE  BLOCK TO BE RETURNED
                            //FOUND SOME SPACE IN PREVIOUS BLOCK SO REQUIRED SIZE FROM NEW BLOCK IS SIZE_1 - BLOCK SIZE
                        remaining_block = remaining_block + (cursor->header.block_size << 4);
                        seg_free_list[i].head = cursor->next;
                        colBlock = (temp_header->block_size << 4);
                        break;
                    }
                }
            }
            sf_header *header3 = (sf_header*)((char*)temp_header);
            header3-> allocated = 0;
            header3-> padded = 0;
            header3-> two_zeroes = 0;

            header3-> block_size = (colBlock + 4096) >> 4;
            header3-> unused = 0;

            sf_footer *footer3 = (void*)((char*)header3 + ((header3->block_size) << 4) - 8);
            footer3-> allocated = 0;
            footer3-> padded = 0;
            footer3-> two_zeroes = 0;
            footer3-> block_size = (colBlock + 4096) >> 4;
            footer3-> requested_size = 0;
            for(int i = 0; i < 4;i++){
                if(seg_free_list[i].min <= (header3->block_size << 4) && seg_free_list[i].max >= (header3->block_size <<4)){
                    ((sf_free_header*)header3)->next = seg_free_list[i].head;
                    if(!(seg_free_list[i].head == NULL)){
                        seg_free_list[i].head->prev = (sf_free_header*)header3;
                    }
                    seg_free_list[i].head = (sf_free_header*)header3;
                }
            }
            goto here;
            // if(space >=size_1){
            //     enough = 1;
            // }
        }
    }
    if(flag == 1){
        temp_header = &cursor1->header;
        remaining_block = (cursor1->header.block_size << 4) - size_1;
    }
    header1 = temp_header;
    header1->allocated = 1;
    header1-> padded = padding;
    header1->two_zeroes = 0;
    if(remaining_block <= 16){
        header1->block_size = space >> 4;
        padding = 1;
        header1-> padded = padding;
    }else{
        header1->block_size = size_1 >> 4;
    }
    pay_load = (char*)header1 + 8;
    sf_footer *footer;
    if(remaining_block <=16){
        footer = (sf_footer*)((char*)header1 + space - 8);
        padding = 1;
    }else{
        footer = (sf_footer*)((char*)header1 + size_1 - 8);
    }
    footer->allocated = 1;
    footer-> padded = padding;
    footer->two_zeroes = 0;
    if(remaining_block <= 16){
        footer->block_size = space >> 4;
    }else{
        footer->block_size = size_1 >> 4;
    }
    pay_load = (char*)header1 + 8;
    footer->requested_size = size;
    int i = 0;
    for(i = 0; i < 4;i++){
        if(seg_free_list[i].min <= remaining_block && seg_free_list[i].max >= remaining_block){
            break;
        }
    }
    if(!(remaining_block <= 16)){
        sf_header *header2 = (sf_header*)((char*)footer + 8);
        header2->allocated = 0;
        header2-> padded = 0;
        header2->two_zeroes = 0;
        header2->block_size = remaining_block >> 4;
        header2->unused = 0;

        sf_footer *footer2 = (void*)((char*)header2 + ((header2->block_size) << 4) - 8);
        footer2->allocated = 0;
        footer2-> padded = 0;
        footer2->two_zeroes = 0;
        footer2->block_size = remaining_block >> 4;
        footer2->requested_size = 0;

        ((sf_free_header*)header2)->next = NULL;
        ((sf_free_header*)header2)->prev = NULL;
        if(seg_free_list[i].head == NULL){
            seg_free_list[i].head = (sf_free_header*)header2;
        }else{
            ((sf_free_header*)header2)->next = seg_free_list[i].head;
            seg_free_list[i].head->prev = (sf_free_header*)header2;
            seg_free_list[i].head = (sf_free_header*)header2;
        }
    }
    sf_blockprint(header1);
    sf_snapshot();
    return pay_load;
}
void *sf_realloc(void *ptr, size_t size) {
    //BLOCK IS ALREADY FREE
    ptr_check(ptr);
    sf_header *header = (sf_header*)((char*)ptr - 8);
    sf_footer *footer = (sf_footer*)((char*)header + (header->block_size << 4) - 8);
    void *ret;
    int padding = 0;
    if(size == 0){
        sf_free(ptr);
        return NULL;
    }
    int size_1 = (int)size + 8 + 8;
    if(size_1 % 16 != 0){
        size_1 = size_1 + (16 - size_1%16);
        padding = 1;
    }
    if(size_1 > (header->block_size <<4)){
        ret = sf_malloc(size);
        if(ret != NULL){
            memcpy(ret, ptr, header->block_size << 4);
            sf_free(ptr);
        }
    }else{
        //SPLINTER CREATED SO DONT SPLIT
        if((header->block_size << 4) - size_1 < 32){
            return ptr;
        }
        //REMAINING BLOCK IS BIGGER AND SMALLER NEED TO BE ADDED TO LIST
        else{
            int remaining_block =(header->block_size << 4) - size_1;
            header->allocated = 1;
            header->padded = padding;
            header->two_zeroes = 0;
            header->block_size = size_1 >> 4;
            header->unused = 0;

            footer = (sf_footer*)((char*)header + (header->block_size << 4) - 8);
            footer->allocated = 1;
            footer-> padded = padding;
            footer->two_zeroes = 0;
            footer->block_size = size_1 >> 4;
            footer->requested_size = size;

            sf_header *header2 = (sf_header*)((char*)footer + 8);
            header2->allocated = 1;
            header2-> padded = 1;
            header2->two_zeroes = 0;
            header2->block_size = remaining_block >> 4;
            header2->unused = 0;

            sf_footer *footer2 = (void*)((char*)header2 + ((header2->block_size) << 4) - 8);
            footer2->allocated = 1;
            footer2-> padded = 1;
            footer2->two_zeroes = 0;
            footer2->block_size = remaining_block >> 4;
            footer2->requested_size = 0;
            sf_free((char*)header2 + 8);
            ret = (char*)header + 8;
        }
    }
    return ret;
}
void sf_free(void *ptr) {
    sf_header *header = (sf_header*)((char*)ptr - 8);
    sf_footer *footer = (sf_footer*)((char*)header + (header->block_size << 4) - 8);
    int block_size1 = 0;
    //BLOCK IS ALREADY FREE
    ptr_check(ptr);
    header->allocated = 0;
    header-> padded = 0;
    header->two_zeroes = 0;
    ((sf_free_header*)header)->next = NULL;
    ((sf_free_header*)header)->prev = NULL;
    header->unused = 0;

    footer->allocated = 0;
    footer-> padded = 0;
    footer->two_zeroes = 0;
    footer->requested_size = 0;
    block_size1 = header->block_size << 4;
    sf_header *header2 = (sf_header*)((char*)footer + 8);
    if(header2->allocated == 0){
        block_size1 = (header->block_size << 4) + (header2->block_size << 4);
        footer = (sf_footer*)((char*)header2 + (header2->block_size << 4) - 8);
        header->block_size = block_size1 >> 4;
        footer->block_size = block_size1 >> 4;
    }
    for(int i = 0;i < 4;i++){
        sf_free_header *cursor = seg_free_list[i].head;
        while(cursor != NULL){
            if(&(cursor->header) == header2){
                if(cursor->prev!=NULL){
                    (cursor->prev)->next = cursor->next;
                    (cursor->next)->prev = cursor->prev;
                }else{
                    seg_free_list[i].head = seg_free_list[i].head->next;
                }
            }
            cursor = cursor->next;
        }
    }
    int i = 0;
    for(i = 0; i < 4;i++){
        if(seg_free_list[i].min <= block_size1 && seg_free_list[i].max >= block_size1){
            ((sf_free_header*)header)->next = seg_free_list[i].head;
            seg_free_list[i].head = (sf_free_header*)header;
        }
    }
    sf_snapshot();
    return;
}
void ptr_check(void *ptr){
    if(ptr == NULL){
        abort();
    }
    sf_header *header = (sf_header*)((char*)ptr - 8);
    if(header->allocated == 0){
        abort();
    }
    sf_footer *footer = (sf_footer*)((char*)header + (header->block_size << 4) - 8);
    if(footer->allocated == 0){
        abort();
    }
    else if(footer->requested_size + 16 != footer->block_size << 4){
        if(!(footer->padded == 1)){
            abort();
        }
    }
    else if(header->allocated != footer->allocated && header->padded != footer->padded){
        abort();
    }
}
