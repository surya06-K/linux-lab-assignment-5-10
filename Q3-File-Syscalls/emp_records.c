/* ============================================================
   Q3: Secure file-processing utility using raw Linux syscalls
   - Creates a file and writes fixed-size employee records
   - Updates a specific record in place (no full-file rewrite)
   - Retrieves any record directly using lseek() offset math
   Uses only open()/read()/write()/lseek()/close() -- no stdio
   file functions (fopen/fread/fwrite) are used for file I/O.
   ============================================================ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>

#define NAME_LEN 50

typedef struct {
    int emp_id;
    char name[NAME_LEN];
    float salary;
} Employee;

static void write_record(int fd, Employee *e) {
    ssize_t n = write(fd, e, sizeof(Employee));
    if (n != (ssize_t)sizeof(Employee)) {
        perror("write failed");
        exit(1);
    }
}

static void print_record(int index, Employee *e) {
    printf("  Record %d -> ID: %d | Name: %-20s | Salary: %.2f\n",
           index, e->emp_id, e->name, e->salary);
}

int main(void) {
    const char *filename = "employees.dat";
    int fd;
    Employee e;

    fd = open(filename, O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (fd < 0) { perror("open (create) failed"); exit(1); }

    Employee employees[5] = {
        {101, "Ravi Kumar",   45000.00},
        {102, "Priya Singh",  52000.00},
        {103, "Kiran Rao",    38000.00},
        {104, "Anjali Mehta", 61000.00},
        {105, "Suresh Nair",  47000.00}
    };

    printf("[Step 1] Creating '%s' and writing 5 employee records...\n", filename);
    for (int i = 0; i < 5; i++) write_record(fd, &employees[i]);
    close(fd);
    printf("[Step 1] Done. Record size = %zu bytes, file size = %zu bytes.\n\n",
           sizeof(Employee), 5 * sizeof(Employee));

    fd = open(filename, O_RDONLY);
    if (fd < 0) { perror("open (read) failed"); exit(1); }

    printf("[Step 2] Reading all records BEFORE update:\n");
    int idx = 0;
    while (read(fd, &e, sizeof(Employee)) == (ssize_t)sizeof(Employee)) {
        print_record(idx, &e);
        idx++;
    }
    close(fd);
    printf("\n");

    int update_index = 2;
    Employee updated = {103, "Kiran Rao", 55000.00};

    fd = open(filename, O_RDWR);
    if (fd < 0) { perror("open (update) failed"); exit(1); }

    off_t offset = (off_t)update_index * sizeof(Employee);
    if (lseek(fd, offset, SEEK_SET) == (off_t)-1) { perror("lseek failed"); exit(1); }
    write_record(fd, &updated);
    close(fd);
    printf("[Step 3] Updated record %d (emp_id 103) salary -> %.2f (only %zu bytes touched, not the whole file)\n\n",
           update_index, updated.salary, sizeof(Employee));

    int lookup_index = 4;
    fd = open(filename, O_RDONLY);
    if (fd < 0) { perror("open (lookup) failed"); exit(1); }

    offset = (off_t)lookup_index * sizeof(Employee);
    lseek(fd, offset, SEEK_SET);
    read(fd, &e, sizeof(Employee));
    printf("[Step 4] Direct lookup of record %d (jumped straight there, no prior records read):\n", lookup_index);
    print_record(lookup_index, &e);
    printf("\n");
    close(fd);

    fd = open(filename, O_RDONLY);
    printf("[Step 5] Reading all records AFTER update (only record 2 should differ):\n");
    idx = 0;
    while (read(fd, &e, sizeof(Employee)) == (ssize_t)sizeof(Employee)) {
        print_record(idx, &e);
        idx++;
    }
    close(fd);

    return 0;
}
