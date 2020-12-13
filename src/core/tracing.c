#include <mgba/core/tracing.h>

#define BUFFER_INITIAL_SIZE (1 << 19);

static bool tracing_enabled = false;
static char* trace_buffer = NULL;
static size_t trace_buffer_capacity = BUFFER_INITIAL_SIZE;
static size_t trace_buffer_size = 0;

static void realloc_for_event(size_t event_object_size)
{
    // + 1 because there is one byte for the event type before the event payload.
    if (trace_buffer_size + (event_object_size + 1) >= trace_buffer_capacity)
    {
        trace_buffer_capacity *= 2;
        trace_buffer = realloc(trace_buffer, trace_buffer_capacity);
    }
}

bool cpu_tracing_enabled(void)
{
    return tracing_enabled;
}

bool cpu_tracing_start(void)
{
    // Tracing was already started
    if (tracing_enabled)
        return false;

    if (!trace_buffer)
    {
        trace_buffer_capacity = BUFFER_INITIAL_SIZE;
        trace_buffer = malloc(trace_buffer_capacity);
    }

    trace_buffer_size = 0;
    tracing_enabled = true;

    return true;
}

bool cpu_tracing_save(const char* path)
{
    if (!tracing_enabled)
        return false;

    FILE* fp = fopen(path, "wb");

    if (!fp)
        return false;

    fwrite(trace_buffer, trace_buffer_size, 1, fp);
    fclose(fp);

    // Reset the tracing state
    trace_buffer_size = 0;
    tracing_enabled = false;

    return true;
}

void cpu_tracing_add_read(uint32_t pc, uint32_t address, uint32_t value, uint8_t size)
{
    if (!tracing_enabled)
        return;

    struct TraceMemoryEvent event =
    {
        .pc = pc,
        .address = address,
        .value = value,
        .size = size
    };

    realloc_for_event(sizeof(event));
    trace_buffer[trace_buffer_size++] = EVENT_READ;
    memcpy(trace_buffer + trace_buffer_size, &event, sizeof(event));
    trace_buffer_size += sizeof(event);
}

void cpu_tracing_add_write(uint32_t pc, uint32_t address, uint32_t value, uint8_t size)
{
    if (!tracing_enabled)
        return;

    struct TraceMemoryEvent event =
    {
        .pc = pc,
        .address = address,
        .value = value,
        .size = size
    };

    realloc_for_event(sizeof(event));
    trace_buffer[trace_buffer_size++] = EVENT_WRITE;
    memcpy(trace_buffer + trace_buffer_size, &event, sizeof(event));
    trace_buffer_size += sizeof(event);
}

void cpu_tracing_add_edge(uint32_t source, uint32_t destination, enum TraceEdgeType type)
{
    if (!tracing_enabled)
        return;

    struct TraceEventEdge event =
    {
        .source = source,
        .target = destination,
        .type = type
    };

    realloc_for_event(sizeof(event));
    trace_buffer[trace_buffer_size++] = EVENT_EDGE;
    memcpy(trace_buffer + trace_buffer_size, &event, sizeof(event));
    trace_buffer_size += sizeof(event);
}
