#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#define PAGE_SIZE 8
#define NUM_OF_PAGES 25
#define MEMORY_SIZE 40
#define SWAP_SIZE 200

int cur_swap=0;
int cur_frame=0;
int old_frame=0;
int old_page=0;
int total_frames=0;
typedef struct page_descriptor {
    unsigned int V; // valid
    unsigned int D; // dirty
    unsigned int P; // permission
    unsigned int frame_swap; // the number of a frame/swap if in case it is page-mapped
} page_descriptor;

typedef struct sim_database {
    page_descriptor page_table[NUM_OF_PAGES];
    int swapfile_fd; // swap file fd
    int program_fd; // executable file fd
    char main_memory[MEMORY_SIZE];
    int text_size;
    int data_size;
    int bss_heap_stack_size;
} sim_database;

sim_database* init_system(char exe_file_name[], char swap_file_name[], int text_size, int data_size, int bss_heap_stack_size) {
    sim_database* mem_sim = (sim_database*)malloc(sizeof(sim_database));
    if (!mem_sim) {
        perror("Error allocating memory");
        exit(EXIT_FAILURE);
    }

    // Open the executable file
    mem_sim->program_fd = open(exe_file_name, O_RDONLY);
    if (mem_sim->program_fd < 0) {
        perror("Error opening executable file");
        free(mem_sim);
        return NULL;
    }

    // Open the swap file
    mem_sim->swapfile_fd = open(swap_file_name, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    if (mem_sim->swapfile_fd < 0) {
        perror("Error opening swap file");
        close(mem_sim->program_fd);
        free(mem_sim);
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < SWAP_SIZE; i++) {
        write(mem_sim->swapfile_fd, "0", sizeof (char));
    }
    // Initialize main memory and page table
    memset(mem_sim->main_memory, 0, MEMORY_SIZE);
    for (int i = 0; i < NUM_OF_PAGES; i++) {
        mem_sim->page_table[i].V = 0;
        mem_sim->page_table[i].D = 0;
        mem_sim->page_table[i].P = (i < text_size / PAGE_SIZE) ? 1 : 0; // Read-only for text segment
        mem_sim->page_table[i].frame_swap = -1;
    }

    mem_sim->text_size = text_size;
    mem_sim->data_size = data_size;
    mem_sim->bss_heap_stack_size = bss_heap_stack_size;

    return mem_sim;
}

int search_old_position(sim_database* mem_sim){
    for (int i=0; i<NUM_OF_PAGES; i++){
        page_descriptor page=mem_sim->page_table[i];
        if (page.frame_swap==old_frame && page.V){
            return i;
        }
    }
    return -1;
}

void clean_old_position(sim_database* mem_sim) {
    if (old_frame==5){
        old_frame=0;
    }
    old_page= search_old_position(mem_sim);

    if (mem_sim->page_table[old_page].V == 1) {
        if (mem_sim->page_table[old_page].D == 1) {
            // Write dirty pages to swap file
            lseek(mem_sim->swapfile_fd, cur_swap * PAGE_SIZE, SEEK_SET);
            write(mem_sim->swapfile_fd, &mem_sim->main_memory[old_frame * PAGE_SIZE], PAGE_SIZE);
            mem_sim->page_table[old_page].frame_swap = cur_swap;
            cur_swap++;
        } else {
            mem_sim->page_table[old_page].frame_swap = -1;
        }
        mem_sim->page_table[old_page].V = 0;
    }
    old_frame++;
}

char load(sim_database* mem_sim, int address) {
    int page_number = address >> 3;
    int offset = 7 & address;
    if (address> mem_sim->bss_heap_stack_size+mem_sim->text_size+mem_sim->text_size){
        fprintf(stderr, "Error: Invalid address %d\n", address);
        return 0;
    }

    // Check for a valid address
    if (page_number >= NUM_OF_PAGES || page_number < 0) {
        fprintf(stderr, "Error: Invalid address %d\n", address);
        return 0;
    }

    page_descriptor* page = &mem_sim->page_table[page_number];

    if (page->V == 1) {
        return mem_sim->main_memory[page->frame_swap * PAGE_SIZE + offset];
    }
    if (address >= (mem_sim->data_size+mem_sim->text_size)) {
        //address in the heap-bss-stack
        if (total_frames==5){
            total_frames--;
            clean_old_position(mem_sim);
            cur_frame=old_frame-1;

        }
        if (page->D==1){
            lseek(mem_sim->swapfile_fd, page->frame_swap * PAGE_SIZE, SEEK_SET);
            read(mem_sim->swapfile_fd, &mem_sim->main_memory[cur_frame * PAGE_SIZE], PAGE_SIZE);
            lseek(mem_sim->swapfile_fd, page->frame_swap * PAGE_SIZE, SEEK_SET);
            size_t size = sizeof (char);
            for (int i=0; i<PAGE_SIZE; i++){
                write(mem_sim->swapfile_fd, "0", size);
            }
            page->V = 1;
            page->frame_swap = cur_frame;

            // Return the value from the main memory
            cur_frame++;

            return mem_sim->main_memory[cur_frame-1 * PAGE_SIZE + offset];
        }

        //create page for heap-bss-stack if it is not found in the swap
        // Create a new page and frame, set chars of the frame to zeros
        memset(&mem_sim->main_memory[cur_frame * PAGE_SIZE], '0', PAGE_SIZE);
        // Update the page table entry
        mem_sim->page_table[page_number].V = 1;
        mem_sim->page_table[page_number].D = 0;
        mem_sim->page_table[page_number].frame_swap = cur_frame;
        cur_frame++;
        total_frames++;
        return mem_sim->main_memory[cur_frame-1 * PAGE_SIZE + offset];
    }

    if (total_frames==5){
        total_frames--;
        clean_old_position(mem_sim);
        cur_frame=old_frame-1;

    }

    if (page->D == 0) {
        // Load from executable file
        lseek(mem_sim->program_fd, page_number * PAGE_SIZE, SEEK_SET);
        read(mem_sim->program_fd, &mem_sim->main_memory[cur_frame * PAGE_SIZE], PAGE_SIZE);
    } else if (page->D == 1) {
        // Load from swap file
        lseek(mem_sim->swapfile_fd, page->frame_swap * PAGE_SIZE, SEEK_SET);
        read(mem_sim->swapfile_fd, &mem_sim->main_memory[cur_frame * PAGE_SIZE], PAGE_SIZE);
        lseek(mem_sim->swapfile_fd, page->frame_swap * PAGE_SIZE, SEEK_SET);
        size_t size = sizeof (char);
        for (int i=0; i<PAGE_SIZE; i++){
            write(mem_sim->swapfile_fd, "0", size);
        }
    }

    // Update the page table entry
    page->V = 1;
    page->frame_swap = cur_frame;

    // Return the value from the main memory
    cur_frame++;
    total_frames++;
    return mem_sim->main_memory[cur_frame-1 * PAGE_SIZE + offset];
}

void store(struct sim_database* mem_sim, int address, char value) {
    int page_number = address >> 3;
    int offset = 7 & address;

    if (address> mem_sim->bss_heap_stack_size+mem_sim->text_size+mem_sim->text_size){
        fprintf(stderr, "Error: Invalid address %d\n", address);
        return;
    }

    if (mem_sim->page_table[page_number].P==1){
        fprintf(stderr, "Error: READ ONLY ADDRESS - trying to write to the text %d\n", address);
        return;
    }

    // Check for a valid address
    if (page_number >= NUM_OF_PAGES || page_number < 0) {
        fprintf(stderr, "Error: Invalid address %d\n", address);
        return;
    }

    page_descriptor* page = &mem_sim->page_table[page_number];

    // If the page is already in memory
    if (page->V == 1) {
        mem_sim->main_memory[page->frame_swap * PAGE_SIZE + offset] = value;
        page->D = 1;
        return;
    }

    if (address >= (mem_sim->data_size+mem_sim->text_size)) {
        //the address is in the heap-bss-stack
        if (total_frames==5){
            total_frames--;
            clean_old_position(mem_sim);
            cur_frame=old_frame-1;

        }
        if (page->D==1){
            lseek(mem_sim->swapfile_fd, page->frame_swap * PAGE_SIZE, SEEK_SET);
            read(mem_sim->swapfile_fd, &mem_sim->main_memory[cur_frame * PAGE_SIZE], PAGE_SIZE);
            lseek(mem_sim->swapfile_fd, page->frame_swap * PAGE_SIZE, SEEK_SET);
            size_t size = sizeof (char);
            for (int i=0; i<PAGE_SIZE; i++){
                write(mem_sim->swapfile_fd, "0", size);
            }
            page->V = 1;
            page->frame_swap = cur_frame;

            // Return the value from the main memory
            mem_sim->main_memory[cur_frame * PAGE_SIZE + offset]=value;
            cur_frame++;
            total_frames++;
            return;
        }

        memset(&mem_sim->main_memory[cur_frame * PAGE_SIZE], '0', PAGE_SIZE);
        // Update the page table entry
        mem_sim->page_table[page_number].V = 1;
        mem_sim->page_table[page_number].D = 1;
        mem_sim->page_table[page_number].frame_swap = cur_frame;

        mem_sim->main_memory[cur_frame * PAGE_SIZE + offset]=value;
    }

    // If the page is not in memory, we need to load it
    if (total_frames==5){
        total_frames--;
        clean_old_position(mem_sim);
        cur_frame=old_frame-1;
    }

    if (address >= mem_sim->text_size && address < (mem_sim->data_size+mem_sim->text_size)) {
        // the address is in the data
        if (page->D == 0) {
            // Load from executable file
            lseek(mem_sim->program_fd, page_number * PAGE_SIZE, SEEK_SET);
            read(mem_sim->program_fd, &mem_sim->main_memory[cur_frame * PAGE_SIZE], PAGE_SIZE);
        } else if (page->D == 1) {
            // Load from swap file
            lseek(mem_sim->swapfile_fd, page->frame_swap * PAGE_SIZE, SEEK_SET);
            read(mem_sim->swapfile_fd, &mem_sim->main_memory[cur_frame * PAGE_SIZE], PAGE_SIZE);
            lseek(mem_sim->swapfile_fd, page->frame_swap * PAGE_SIZE, SEEK_SET);
            size_t size = sizeof (char);
            for (int i=0; i<PAGE_SIZE; i++){
                write(mem_sim->swapfile_fd, "0", size);
            }
        }
    }

    // Update the page table entry
    page->V = 1;
    page->frame_swap = cur_frame;
    mem_sim->main_memory[cur_frame * PAGE_SIZE + offset] = value;
    page->D = 1;
    cur_frame++;
    total_frames++;
}


void print_swap(sim_database* mem_sim) {
    char str[PAGE_SIZE];
    int i;
    printf("\n Swap memory\n");
    lseek(mem_sim->swapfile_fd, 0, SEEK_SET); // go to the start of the file
    while(read(mem_sim->swapfile_fd, str, PAGE_SIZE) == PAGE_SIZE) {
        for(i = 0; i < PAGE_SIZE; i++) {
            printf("[%c]\t", str[i]);
        }
        printf("\n");
    }
}
void print_page_table(sim_database* mem_sim) {
    int i;
    printf("\n page table \n");
    printf("Valid\t Dirty\t Permission \t Frame_swap\n");
    for(i = 0; i < NUM_OF_PAGES; i++) {
        printf("[%d]\t[%d]\t[%d]\t[%d]\n", mem_sim->page_table[i].V,
               mem_sim->page_table[i].D,
               mem_sim->page_table[i].P, mem_sim->page_table[i].frame_swap);
    }
}
void print_memory(sim_database* mem_sim) {
    int i;
    printf("\n Physical memory\n");
    for(i = 0; i < MEMORY_SIZE; i++) {
        printf("[%c]\n", mem_sim->main_memory[i]);
    }
}

void clear_system(sim_database* mem_sim) {
    close(mem_sim->program_fd);
    close(mem_sim->swapfile_fd);
    free(mem_sim);
}

