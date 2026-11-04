/* SPDX-License-Identifier: MIT */
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "libfdt/libfdt.h"
#include "soc.h"

typedef uint32_t u32;
static unsigned char tree[65536];
static void *dt = tree;
static void *adt;
static u32 chip_id;
static u32 board = 10, vendor = 6, capacity = 8;
static const unsigned char tunable[] = {0x19, 0x83, 0x24, 0x55, 0x80};
static struct property { const char *name; const void *value; u32 size; } props[] = {
    {"controller", tunable, sizeof(tunable)},
    {"name", "iop-pmp-nub", 12},
    {"dram-capacity", &capacity, sizeof(capacity)},
};
#define ADT_FOREACH_PROPERTY(a, n, p) \
    for (struct property *p = props; p < props + sizeof(props) / sizeof(props[0]); p++)
#define bail(...) do { printf(__VA_ARGS__); return -1; } while (0)
static int adt_path_offset(const void *a, const char *path)
{
    (void)a;
    return !strcmp(path, "/chosen") ? 1 :
           !strcmp(path, "/arm-io/pmp/iop-pmp-nub") ? 2 : -1;
}
static int adt_getprop_copy(const void *a, int node, const char *name, void *out, size_t size)
{
    const void *value = NULL;
    (void)a;
    if (node == 1 && !strcmp(name, "board-id")) value = &board;
    if (node == 1 && !strcmp(name, "dram-vendor-id")) value = &vendor;
    if (node == 2 && !strcmp(name, "dram-capacity")) value = &capacity;
    if (!value || size != sizeof(u32)) return -1;
    memcpy(out, value, size);
    return 0;
}
#define ADT_GETPROP(a, n, key, val) adt_getprop_copy(a, n, key, val, sizeof(*(val)))
#include "pmp_under_test.inc"

#define CHECK(c, message) do { if (!(c)) { fprintf(stderr, "FAIL: %s\n", message); return 1; } } while (0)
static int node(const char *name)
{
    int soc = fdt_path_offset(dt, "/soc");
    int n = fdt_add_subnode(dt, soc, name);
    if (n < 0 || fdt_setprop_string(dt, n, "compatible", "apple,t6000-pmp-v2") ||
        fdt_setprop_string(dt, n, "status", "disabled")) abort();
    return n;
}
int main(int argc, char **argv)
{
    CHECK(argc == 2, "one scenario required");
    chip_id = T6001;
    CHECK(fdt_create_empty_tree(dt, sizeof(tree)) == 0, "real FDT initialized");
    CHECK(fdt_add_subnode(dt, 0, "soc") >= 0, "soc created");
    node("pmp@28e700000");
    bool expected = true;
    if (!strcmp(argv[1], "alias")) {
        int aliases = fdt_add_subnode(dt, 0, "aliases");
        CHECK(aliases >= 0 && fdt_setprop_string(dt, aliases, "pmp", "/soc/pmp@28e700000") == 0,
              "existing alias created");
        chip_id = T6021;
    } else if (!strcmp(argv[1], "another-unit")) {
        board = 14; vendor = 3; capacity = 16;
    } else if (!strcmp(argv[1], "ambiguous")) {
        node("pmp@28e710000"); expected = false;
    } else if (!strcmp(argv[1], "other-chip")) {
        chip_id = T8103; expected = false;
    } else if (!strcmp(argv[1], "absent")) {
        CHECK(fdt_del_node(dt, fdt_path_offset(dt, "/soc/pmp@28e700000")) == 0, "node removed");
        expected = false;
    } else {
        CHECK(!strcmp(argv[1], "missing-alias"), "known scenario");
    }
    CHECK(dt_set_pmp() == 0, "handoff returned normally");
    int n = fdt_path_offset(dt, "/soc/pmp@28e700000"), len = 0;
    const fdt32_t *got = n < 0 ? NULL : fdt_getprop(dt, n, "apple,board-id", &len);
    if (!expected) {
        int candidate = -1;
        while ((candidate = fdt_node_offset_by_compatible(dt, candidate, "apple,t6000-pmp-v2")) >= 0) {
            CHECK(!fdt_getprop(dt, candidate, "apple,board-id", NULL), "no candidate was calibrated");
            CHECK(!strcmp(fdt_getprop(dt, candidate, "status", NULL), "disabled"), "every candidate stays disabled");
        }
    } else {
        CHECK(got && len == 4 && fdt32_to_cpu(*got) == board, "this unit board calibration exported");
        got = fdt_getprop(dt, n, "apple,dram-vendor-id", &len);
        CHECK(got && len == 4 && fdt32_to_cpu(*got) == vendor, "this unit DRAM vendor exported");
        got = fdt_getprop(dt, n, "apple,dram-capacity", &len);
        CHECK(got && len == 4 && fdt32_to_cpu(*got) == capacity, "this unit DRAM capacity exported");
        const void *bytes = fdt_getprop(dt, n, "apple,tunable-controller", &len);
        CHECK(bytes && len == sizeof(tunable) && !memcmp(bytes, tunable, len), "tunable bytes unchanged");
        CHECK(!fdt_getprop(dt, n, "apple,tunable-name", NULL), "identity property excluded");
        CHECK(!strcmp(fdt_getprop(dt, n, "status", NULL), "okay"), "calibrated node enabled");
    }
    puts("PASS");
    return 0;
}
