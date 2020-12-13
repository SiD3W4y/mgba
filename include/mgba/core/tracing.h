#ifndef TRACING_H
#define TRACING_H

#include <mgba-util/common.h>

enum TraceEventOpcode
{
    EVENT_EDGE = 0,
    EVENT_READ,
    EVENT_WRITE
};

enum TraceEdgeType
{
    EDGE_BRANCH = 0,
    EDGE_CALL = 1
};

struct __attribute__((packed)) TraceEventEdge
{
    uint32_t source;
    uint32_t target;
    uint8_t type;
};

struct __attribute__((packed)) TraceMemoryEvent
{
    uint32_t pc;
    uint32_t address;
    uint32_t value;
    uint8_t size;
};

/*
 * Returns whether tracing is enabled.
 */
bool cpu_tracing_enabled(void);

/*
 * Enables and starts cpu state tracing.
 */
bool cpu_tracing_start(void);

/*
 * Saves the trace buffer to a file.
 */
bool cpu_tracing_save(const char* path);

/*
 * Adds an edge event to the trace.
 */
void cpu_tracing_add_edge(uint32_t source, uint32_t destination, enum TraceEdgeType type);

/*
 * Adds a memory read event to the trace.
 */
void cpu_tracing_add_read(uint32_t pc, uint32_t address, uint32_t value, uint8_t size);

/*
 * Adds a memory write event to the trace.
 */
void cpu_tracing_add_write(uint32_t pc, uint32_t address, uint32_t value, uint8_t size);

#endif
