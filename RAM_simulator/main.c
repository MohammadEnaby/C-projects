#include <stdio.h>
#include "mem_sim.h"


int main() {
    sim_database *mem_sim = init_system("exec_file", "swap_file", 40, 40, 120);
    store(mem_sim, 40, 'H');
    store(mem_sim, 50, 'F');
    store(mem_sim, 60, 'Y');
    store(mem_sim, 70, '&');
    store(mem_sim, 80, 'A');
    load(mem_sim, 140);

    print_memory(mem_sim);
    print_page_table(mem_sim);
    print_swap(mem_sim);
    clear_system(mem_sim);
    return 0;
}
