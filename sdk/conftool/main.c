#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "gcf.h"

struct flag_name {
    const char *name;
    uint16_t flag;
};

static const struct flag_name flag_names[] = {
    {"s", GCF_ENTRY_SYSTEM},
};

static uint16_t flag_name_to_bit(const char *name, size_t length) {
    for (size_t i = 0; i < sizeof(flag_names) / sizeof(flag_names[0]); i++) {
        if (strlen(flag_names[i].name) == length && memcmp(flag_names[i].name, name, length) == 0) { return flag_names[i].flag; }
    }
    return 0;
}

static void print_flags(uint16_t flags) {
    int first = 1;
    for (size_t i = 0; i < sizeof(flag_names) / sizeof(flag_names[0]); i++) {
        if ((flags & flag_names[i].flag) == 0) { continue; }
        printf("%s%s", first ? "" : ",", flag_names[i].name);
        first = 0;
    }
}

static void help(const char *name) {
    printf("usage: %s -f <file> <command>\n", name);
    printf("commands:\n");
    printf("- list\n");
    printf("- get <name>\n");
    printf("- set <name> <value>\n");
    printf("- delete <name>\n");
    printf("- flags <name> [<spec>]\n");
    printf("  spec: [+|-|=]<flag>[...] (e.g. +s, =)\n");
    printf("- compact\n");
}

static int read_file(const char *path, void **data, size_t *size) {
    int fd = open(path, O_RDONLY);
    if (fd == -1) { return -1; }

    struct stat sb;
    if (fstat(fd, &sb) == -1) {
        int err = errno;
        close(fd);
        errno = err;
        return -1;
    }
    if (sb.st_size < 0 || (uintmax_t)sb.st_size > (uintmax_t)SIZE_MAX) {
        close(fd);
        errno = EFBIG;
        return -1;
    }

    size_t file_size = (size_t)sb.st_size;
    void *buf = malloc(file_size > 0 ? file_size : 1);
    if (buf == NULL) {
        int err = errno;
        close(fd);
        errno = err;
        return -1;
    }

    size_t done = 0;
    while (done < file_size) {
        ssize_t ret = read(fd, (char*)buf + done, file_size - done);
        if (ret <= 0) {
            int err = ret == 0 ? EIO : errno;
            free(buf);
            close(fd);
            errno = err;
            return -1;
        }
        done += (size_t)ret;
    }

    close(fd);
    *data = buf;
    *size = file_size;
    return 0;
}

static int write_file(const char *path, const void *data, size_t size) {
    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) { return -1; }

    size_t done = 0;
    while (done < size) {
        ssize_t ret = write(fd, (const char*)data + done, size - done);
        if (ret <= 0) {
            int err = ret == 0 ? EIO : errno;
            close(fd);
            errno = err;
            return -1;
        }
        done += (size_t)ret;
    }

    if (close(fd) == -1) { return -1; }
    return 0;
}

static int check_file(const void *data, size_t size) {
    switch (gcf_is_valid(data, size)) {
        case 0:
            return 0;
        case 1:
            fprintf(stderr, "not a GCF file\n");
            return -1;
        case 2:
            fprintf(stderr, "unsupported GCF version\n");
            return -1;
        default:
            fprintf(stderr, "corrupt GCF file\n");
            return -1;
    }
}

static int cmd_list(const char *path) {
    void *data;
    size_t size;
    if (read_file(path, &data, &size) < 0) {
        perror(path);
        return 1;
    }
    if (check_file(data, size) < 0) {
        free(data);
        return 1;
    }

    const struct gcf_header *h = data;
    for (uint32_t i = 0; i < h->entries_count; i++) {
        size_t name_length = gcf_get_entry_name_length(data, size, i);
        const char *name = gcf_get_entry_name(data, size, i);
        if (name == NULL) { continue; }

        const void *value = gcf_get_entry_data(data, size, i);
        size_t data_length = gcf_get_entry_data_length(data, size, i);

        uint16_t flags = gcf_get_entry_flags(data, size, i);
        if (flags != 0) { putchar('['); print_flags(flags); printf("] "); }
        printf("%.*s = ", (int)name_length, name);
        if (value != NULL) { fwrite(value, 1, data_length, stdout); }
        putchar('\n');
    }

    free(data);
    return 0;
}

static int cmd_get(const char *path, const char *name) {
    void *data;
    size_t size;
    if (read_file(path, &data, &size) < 0) {
        perror(path);
        return 1;
    }
    if (check_file(data, size) < 0) {
        free(data);
        return 1;
    }

    size_t index = gcf_find_entry(data, size, name, strlen(name));
    if (index == SIZE_MAX) {
        fprintf(stderr, "%s: no such entry\n", name);
        free(data);
        return 1;
    }

    const void *value = gcf_get_entry_data(data, size, index);
    size_t data_length = gcf_get_entry_data_length(data, size, index);
    if (value != NULL) { fwrite(value, 1, data_length, stdout); }
    putchar('\n');

    free(data);
    return 0;
}

static int cmd_set(const char *path, const char *name, const char *value) {
    void *data = NULL;
    size_t size = 0;
    if (read_file(path, &data, &size) < 0) {
        if (errno != ENOENT) {
            perror(path);
            return 1;
        }
        data = gcf_create(GCF_DEFAULT_CAPACITY, &size);
        if (data == NULL) {
            perror("gcf_create");
            return 1;
        }
    } else if (check_file(data, size) < 0) {
        free(data);
        return 1;
    }

    size_t name_length = strlen(name);
    size_t value_length = strlen(value);
    size_t index = gcf_find_entry(data, size, name, name_length);

    int ret;
    if (index == SIZE_MAX) {
        ret = gcf_add_entry(&data, &size, name, name_length, value, value_length, NULL);
    } else {
        ret = gcf_set_entry_data(&data, &size, index, value, value_length);
    }
    if (ret < 0) {
        perror("set");
        free(data);
        return 1;
    }

    if (write_file(path, data, gcf_file_size(data)) < 0) {
        perror(path);
        free(data);
        return 1;
    }

    free(data);
    return 0;
}

static int cmd_delete(const char *path, const char *name) {
    void *data;
    size_t size;
    if (read_file(path, &data, &size) < 0) {
        perror(path);
        return 1;
    }
    if (check_file(data, size) < 0) {
        free(data);
        return 1;
    }

    size_t index = gcf_find_entry(data, size, name, strlen(name));
    if (index == SIZE_MAX) {
        fprintf(stderr, "%s: no such entry\n", name);
        free(data);
        return 1;
    }
    if (gcf_delete_entry(data, size, index) < 0) {
        perror("delete");
        free(data);
        return 1;
    }

    if (write_file(path, data, gcf_file_size(data)) < 0) {
        perror(path);
        free(data);
        return 1;
    }

    printf("(%s) deleted\n", name);
    free(data);
    return 0;
}

static int apply_flag_spec(const char *spec, uint16_t *flags_io) {
    if (*spec == '\0') {
        fprintf(stderr, "flags: empty spec\n");
        return -1;
    }

    uint16_t flags = *flags_io;
    const char *p = spec;
    while (*p) {
        if (*p == ',') { p++; continue; }

        char op = *p++;
        if (op != '+' && op != '-' && op != '=') {
            fprintf(stderr, "flags: expected +, - or =\n");
            return -1;
        }

        const char *start = p;
        while (*p && *p != '+' && *p != '-' && *p != '=' && *p != ',') { p++; }
        size_t length = (size_t)(p - start);

        uint16_t bits = 0;
        if (length > 0) {
            bits = flag_name_to_bit(start, length);
            if (bits == 0) {
                fprintf(stderr, "%.*s: unknown flag\n", (int)length, start);
                return -1;
            }
        } else if (op != '=') {
            fprintf(stderr, "flags: missing flag name\n");
            return -1;
        }

        if (op == '+') {
            flags |= bits;
        } else if (op == '-') {
            flags = (uint16_t)(flags & (uint16_t)~bits);
        } else {
            flags = bits;
        }
    }

    *flags_io = flags;
    return 0;
}

static int cmd_flags(const char *path, const char *name, const char *spec) {
    void *data;
    size_t size;
    if (read_file(path, &data, &size) < 0) {
        perror(path);
        return 1;
    }
    if (check_file(data, size) < 0) {
        free(data);
        return 1;
    }

    size_t index = gcf_find_entry(data, size, name, strlen(name));
    if (index == SIZE_MAX) {
        fprintf(stderr, "%s: no such entry\n", name);
        free(data);
        return 1;
    }

    uint16_t flags = gcf_get_entry_flags(data, size, index);
    if (spec == NULL) {
        print_flags(flags);
        putchar('\n');
        free(data);
        return 0;
    }

    if (apply_flag_spec(spec, &flags) < 0) {
        free(data);
        return 1;
    }
    if (gcf_set_entry_flags(data, size, index, flags) < 0) {
        perror("flags");
        free(data);
        return 1;
    }

    if (write_file(path, data, gcf_file_size(data)) < 0) {
        perror(path);
        free(data);
        return 1;
    }

    free(data);
    return 0;
}

static void print_size_report(size_t before, size_t after) {
    if (after == before) {
        printf("nothing to compact\n");
        return;
    }
    printf("before: %zu bytes\n", before);
    printf("after:  %zu bytes\n", after);
    size_t removed = before - after;
    size_t tenths = (removed * 1000 + before / 2) / before; // 0..1000
    printf("reduction: %zu.%zu%%\n", tenths / 10, tenths % 10);
}

static int cmd_compact(const char *path) {
    void *data;
    size_t size;
    size_t oldsize;
    if (read_file(path, &data, &size) < 0) {
        perror(path);
        return 1;
    }
    if (check_file(data, size) < 0) {
        free(data);
        return 1;
    }

    oldsize = size;
    if (gcf_compact(&data, &size) < 0) {
        perror("compact");
        free(data);
        return 1;
    }

    if (write_file(path, data, gcf_file_size(data)) < 0) {
        perror(path);
        free(data);
        return 1;
    }

    print_size_report(oldsize, size);
    free(data);
    return 0;
}

int main(int argc, char *argv[]) {
    printf("GalaxyOS conftool - version %i GCF\n", GCF_VERSION);

    const char *file = NULL;
    int nargs = 0;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0) {
            help(argv[0]);
            return EXIT_SUCCESS;
        }
        if (strcmp(argv[i], "-f") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "conftool: -f requires an argument\n");
                return EXIT_FAILURE;
            }
            file = argv[++i];
            continue;
        }
        argv[1 + nargs] = argv[i];
        nargs++;
    }

    if (file == NULL) {
        fprintf(stderr, "conftool: -f is required\n");
        help(argv[0]);
        return EXIT_FAILURE;
    }
    if (nargs == 0) {
        help(argv[0]);
        return EXIT_FAILURE;
    }

    const char *command = argv[1];
    int npos = nargs - 1;
    if (strcmp(command, "list") == 0 && npos == 0) {
        return cmd_list(file);
    }
    if (strcmp(command, "get") == 0 && npos == 1) {
        return cmd_get(file, argv[2]);
    }
    if (strcmp(command, "set") == 0 && npos == 2) {
        return cmd_set(file, argv[2], argv[3]);
    }
    if (strcmp(command, "delete") == 0 && npos == 1) {
        return cmd_delete(file, argv[2]);
    }
    if (strcmp(command, "flags") == 0 && (npos == 1 || npos == 2)) {
        return cmd_flags(file, argv[2], npos == 2 ? argv[3] : NULL);
    }
    if (strcmp(command, "compact") == 0 && npos == 0) {
        return cmd_compact(file);
    }

    help(argv[0]);
    return EXIT_FAILURE;
}
