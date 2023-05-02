#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define MAX_LENGTH 256
#define CACHE_SIZE 3
#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW    "\x1b[33m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

typedef struct CacheEntry {
    char domain[MAX_LENGTH];
    char ip[MAX_LENGTH];
    struct CacheEntry* prev;
    struct CacheEntry* next;
} CacheEntry;
typedef struct Cache {
    CacheEntry* entries[CACHE_SIZE];
    CacheEntry* head;
    CacheEntry* tail;
    int size;
} Cache;
unsigned int hash(const char* str) {
    unsigned int hash = 0;
    for (int i = 0; str[i] != '\0'; i++) {
        hash = 31 * hash + str[i];
    }
    return hash % CACHE_SIZE;
}
void init_cache(Cache* cache) {
    for (int i = 0; i < CACHE_SIZE; i++) {
        cache->entries[i] = NULL;
    }
    cache->head = NULL;
    cache->tail = NULL;
    cache->size = 0;
}
int is_valid_cname(const char* cname) {
    FILE* file = fopen("dns.txt", "r");
    if (file == NULL) {
        return 0;
    }
    char line[MAX_LENGTH];
    while (fgets(line, MAX_LENGTH, file) != NULL) {
        char name[MAX_LENGTH];
        char type[MAX_LENGTH];
        sscanf(line, "%s %*s %s", name, type);
        if (strcmp(name, cname) == 0 && strcmp(type, "A") == 0) {
            fclose(file);
            return 1;
        }
    }
    fclose(file);
    return 0;
}
void move_to_front(Cache* cache, CacheEntry* entry) {
    if (entry == cache->head) {
        return;
    }
    if (entry == cache->tail) {
        if (cache->tail->prev != NULL) {
            cache->tail = entry->prev;
            cache->tail->next = NULL;
        }
        else {
            cache->tail = NULL;
        }
    }
    else {
        entry->prev->next = entry->next;
        entry->next->prev = entry->prev;
    }
    entry->prev = NULL;
    entry->next = cache->head;
    if (cache->head != NULL) {
        cache->head->prev = entry;
    }
    cache->head = entry;
}
char* find_in_cache(Cache* cache, const char* domain) {
    unsigned int index = hash(domain);
    CacheEntry* entry = cache->entries[index];
    while (entry != NULL) {
        if (strcmp(entry->domain, domain) == 0) {
            move_to_front(cache, entry);
            return entry->ip;
        }
        entry = entry->next;
    }
    return NULL;
}
void add_to_cache(Cache* cache, const char* domain, const char* ip) {
    if (find_in_cache(cache, domain) != NULL) {
        return;
    }
    unsigned int index = hash(domain);
    CacheEntry* new_entry = (CacheEntry*)malloc(sizeof(CacheEntry));
    strcpy(new_entry->domain, domain);
    strcpy(new_entry->ip, ip);
    new_entry->prev = NULL;
    new_entry->next = cache->head;
    if (cache->head != NULL) {
        cache->head->prev = new_entry;
    }
    cache->head = new_entry;
    if (cache->tail == NULL) {
        cache->tail = new_entry;
    }

    if (cache->size == CACHE_SIZE) {
        CacheEntry* entry_to_remove = cache->tail;
        cache->tail = entry_to_remove->prev;
        if (cache->tail != NULL) {
            cache->tail->next = NULL;
        }
        unsigned int index_to_remove = hash(entry_to_remove->domain);

        if (entry_to_remove == cache->entries[index_to_remove]) {
            cache->entries[index_to_remove] = entry_to_remove->next;
        }
        else {
            CacheEntry* current_entry = cache->entries[index_to_remove];

            while (current_entry != NULL) {
                if (current_entry->next == entry_to_remove) {
                    current_entry->next = entry_to_remove->next;
                    break;
                }
                current_entry = current_entry->next;
            }
        }
        free(entry_to_remove);
    }
    else {
        cache->size++;
    }
    if (cache->entries[index] != NULL) {
        new_entry->next = cache->entries[index];
    }
    cache->entries[index] = new_entry;
}

void get_domain(char* domain) {
    printf(ANSI_COLOR_YELLOW "\nEnter domain name: " ANSI_COLOR_RESET);
    scanf("%s", domain);
}
FILE* open_dns_file() {
    FILE* file = fopen("dns.txt", "r");
    return file;
}
char* find_ip_address(FILE* file, Cache* cache, char* domain) {
    char original_domain[MAX_LENGTH];
    strcpy(original_domain, domain);
    char* cached_ip = find_in_cache(cache, domain);
    if (cached_ip != NULL) {
        printf(ANSI_COLOR_GREEN "\nHIT\n" ANSI_COLOR_RESET);
        printf(ANSI_COLOR_CYAN "IP address: %s\n" ANSI_COLOR_RESET, cached_ip);
        return _strdup(cached_ip);
    }
    printf(ANSI_COLOR_RED "\nMISS\n" ANSI_COLOR_RESET);
    fseek(file, 0, SEEK_SET);
    char line[MAX_LENGTH];
    while (fgets(line, MAX_LENGTH, file) != NULL) {
        char name[MAX_LENGTH];
        char type[MAX_LENGTH];
        char value[MAX_LENGTH];
        sscanf(line, "%s %*s %s %s", name, type, value);
        if (strcmp(name, domain) == 0) {
            if (strcmp(type, "A") == 0) {
                printf(ANSI_COLOR_CYAN "IP address: %s\n" ANSI_COLOR_RESET, value);
                add_to_cache(cache, original_domain, value);
                return _strdup(value);
            }
            else if (strcmp(type, "CNAME") == 0) {
                strcpy(domain, value);
                fseek(file, 0, SEEK_SET);
            }
        }
    }
    return NULL;
}
void show_cache(Cache* cache) {
    printf("\nCache:\n");
    CacheEntry* entry = cache->head;
    while (entry != NULL) {
        printf(ANSI_COLOR_CYAN "%s %s\n" ANSI_COLOR_RESET, entry->domain, entry->ip);
        entry = entry->next;
    }
}
int is_valid_ip(const char* ip) {
    int octets[4];
    int num_octets = sscanf(ip, "%d.%d.%d.%d", &octets[0], &octets[1], &octets[2], &octets[3]);
    if (num_octets != 4) {
        return 0;
    }
    for (int i = 0; i < 4; i++) {
        if (octets[i] < 0 || octets[i] > 255) {
            return 0;
        }
    }
    return 1;
}
int is_duplicate_record(const char* domain, const char* type, const char* value) {
    FILE* file = fopen("dns.txt", "r");
    if (file == NULL) {
        return 0;
    }
    char line[MAX_LENGTH];
    while (fgets(line, MAX_LENGTH, file) != NULL) {
        char name[MAX_LENGTH];
        char record_type[MAX_LENGTH];
        char record_value[MAX_LENGTH];
        sscanf(line, "%s %*s %s %s", name, record_type, record_value);
        if (strcmp(name, domain) == 0 && strcmp(record_type, type) == 0 && strcmp(record_value, value) == 0) {
            fclose(file);
            return 1;
        }
    }
    fclose(file);
    return 0;
}
void find_domains_by_ip() {
    printf(ANSI_COLOR_YELLOW "\nEnter IP address: " ANSI_COLOR_RESET);
    char ip[MAX_LENGTH];
    scanf("%s", ip);
    if (!is_valid_ip(ip)) {
        printf(ANSI_COLOR_RED "Invalid IP address\n\n" ANSI_COLOR_RESET);
        return;
    }
    FILE* file = fopen("dns.txt", "r");
    if (file == NULL) {
        printf(ANSI_COLOR_RED "Could not open file\n\n" ANSI_COLOR_RESET);
        return;
    }
    int found = 0;
    char line[MAX_LENGTH];
    while (fgets(line, MAX_LENGTH, file) != NULL) {
        char name[MAX_LENGTH];
        char type[MAX_LENGTH];
        char value[MAX_LENGTH];
        sscanf(line, "%s %*s %s %s", name, type, value);
        if (strcmp(type, "A") == 0 && strcmp(value, ip) == 0) {
            printf("%s\n", name);
            found = 1;
        }
    }
    fclose(file);
    if (!found) {
        printf(ANSI_COLOR_RED "No domains found\n\n" ANSI_COLOR_RESET);
    }
}
void add_record() {
    printf(ANSI_COLOR_YELLOW "\nEnter domain name: " ANSI_COLOR_RESET);
    char domain[MAX_LENGTH];
    scanf("%s", domain);
    char type[MAX_LENGTH];
    do {
        printf(ANSI_COLOR_YELLOW "Enter record type (A or CNAME): " ANSI_COLOR_RESET);
        scanf("%s", type);
        if (strcmp(type, "A") != 0 && strcmp(type, "CNAME") != 0) {
            printf(ANSI_COLOR_RED "Invalid record type\n" ANSI_COLOR_RESET);
        }
    } while (strcmp(type, "A") != 0 && strcmp(type, "CNAME") != 0);
    char value[MAX_LENGTH];
    if (strcmp(type, "A") == 0) {
        do {
            printf(ANSI_COLOR_YELLOW "Enter record value: " ANSI_COLOR_RESET);
            scanf("%s", value);
            if (!is_valid_ip(value)) {
                printf(ANSI_COLOR_RED "Invalid IP address\n" ANSI_COLOR_RESET);
            }
        } while (!is_valid_ip(value));
    }
    else {
        do {
            printf(ANSI_COLOR_YELLOW "Enter record value: " ANSI_COLOR_RESET);
            scanf("%s", value);
            if (!is_valid_cname(value)) {
                printf(ANSI_COLOR_RED "Invalid CNAME value\n" ANSI_COLOR_RESET);
            }
        } while (!is_valid_cname(value));
    }
    if (is_duplicate_record(domain, type, value)) {
        printf(ANSI_COLOR_RED "Duplicate record\n" ANSI_COLOR_RESET);
        return;
    }
    FILE* file = fopen("dns.txt", "a");
    if (file == NULL) {
        printf(ANSI_COLOR_RED "Could not open file\n\n" ANSI_COLOR_RESET);
        return;
    }
    fprintf(file, "%s IN %s %s\n", domain, type, value);
    fclose(file);
    printf(ANSI_COLOR_GREEN "Record added\n" ANSI_COLOR_RESET);
}
void free_cache(Cache* cache) {
    CacheEntry* entry = cache->head;
    while (entry != NULL) {
        CacheEntry* next_entry = entry->next;
        free(entry);
        entry = next_entry;
    }
}
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