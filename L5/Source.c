#include "func.h"



int main() {
    Cache cache;
    init_cache(&cache);
    int choice;
    do {
        printf("\n1. Find IP by domain\n");
        printf("2. Show cache\n");
        printf("3. Add record\n");
        printf("4. Find domains by IP\n");
        printf("0. Exit\n");
        printf(ANSI_COLOR_YELLOW "\nEnter your choice: " ANSI_COLOR_RESET);
        int result = scanf("%d", &choice);
        if (result != 1) {
            int c;
            while ((c = getchar()) != '\n' && c != EOF);
            continue;
        }
        if (choice == 1) {
            char domain[MAX_LENGTH];
            get_domain(domain);
            char original_domain[MAX_LENGTH];
            strcpy(original_domain, domain);

            FILE* file = open_dns_file();
            if (file == NULL) {
                continue;
            }
            char* ip = find_ip_address(file, &cache, domain);
            if (ip == NULL) {
                printf(ANSI_COLOR_RED "Domain not found\n" ANSI_COLOR_RESET);
            }
            else {
                add_to_cache(&cache, original_domain, ip);
                free(ip);
            }
            fclose(file);
        }
        else if (choice == 2) {
            show_cache(&cache);
        }
        else if (choice == 3) {
            add_record();
        }
        else if (choice == 4) {
            find_domains_by_ip();
        }
    } while (choice != 0);

    free_cache(&cache);

    return 0;
}