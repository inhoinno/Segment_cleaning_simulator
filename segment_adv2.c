#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>

#define SEGMENT_SIZE 1024  // Size of each segment in bytes
#define NUM_SEGMENTS 1024  // Total number of segments

typedef struct {
    int segmentId;
    uint64_t utilization;          // Current utilization of the segment
    uint8_t data[SEGMENT_SIZE];    // Simulated storage space
    int valid[SEGMENT_SIZE];       // Validity map for each byte (1 = valid, 0 = invalid)
    uint64_t invalidatedBytes;     // Tracks invalidated bytes for summary
} Segment;

typedef struct {
    Segment segments[NUM_SEGMENTS];
    uint64_t totalWrites;          // Total write requests processed
    uint64_t totalInvalidated;     // Total invalidated bytes across all segments
} SegmentManager;

typedef struct {
    uint64_t offset;  // Logical Block Address (LBA)
    uint64_t size;    // Size of the write in bytes
} WriteRequest;

// Initialize the segment manager
void initSegmentManager(SegmentManager *manager) {
    for (int i = 0; i < NUM_SEGMENTS; i++) {
        manager->segments[i].segmentId = i;
        manager->segments[i].utilization = 0;
        manager->segments[i].invalidatedBytes = 0;
        memset(manager->segments[i].data, 0, SEGMENT_SIZE);
        memset(manager->segments[i].valid, 0, SEGMENT_SIZE);
    }
    manager->totalWrites = 0;
    manager->totalInvalidated = 0;
}

// Find a segment with sufficient free space
Segment *findSegmentForWrite(SegmentManager *manager, uint64_t size) {
    for (int i = 0; i < NUM_SEGMENTS; i++) {
        if (manager->segments[i].utilization + size <= SEGMENT_SIZE) {
            return &manager->segments[i];
        }
    }
    return NULL; // No available segment
}

// Invalidate old data in the segment
void invalidateOldData(Segment *segment, uint64_t offset, uint64_t size, SegmentManager *manager) {
    for (uint64_t i = offset; i < offset + size; i++) {
        if (i < SEGMENT_SIZE && segment->valid[i]) {
            segment->valid[i] = 0;
            segment->utilization--;
            segment->invalidatedBytes++;
            manager->totalInvalidated++;
        }
    }
}

// Write data to the segment
void writeToSegment(Segment *segment, uint64_t offset, uint64_t size, SegmentManager *manager) {
    for (uint64_t i = offset; i < offset + size; i++) {
        if (i < SEGMENT_SIZE) {
            segment->data[i] = 1;  // Simulated data write
            if (!segment->valid[i]) {
                segment->valid[i] = 1;
                segment->utilization++;
            }
        }
    }
}

// Process a write request
void processWriteRequest(SegmentManager *manager, WriteRequest *request) {
    Segment *targetSegment = findSegmentForWrite(manager, request->size);

    if (targetSegment == NULL) {
        printf("Error: No space available for write request (offset: %lu, size: %lu)\n",
               request->offset, request->size);
        return;
    }

    // Invalidate old data if it is an overwrite request
    invalidateOldData(targetSegment, request->offset, request->size, manager);

    // Write new data
    writeToSegment(targetSegment, request->offset, request->size, manager);

    manager->totalWrites++;
}

// Workload Generator
void generateWorkload(SegmentManager *manager, int numRequests, const char *distribution) {
    WriteRequest request;
    srand(time(NULL));

    for (int i = 0; i < numRequests; i++) {
        if (strcmp(distribution, "uniform") == 0) {
            // Uniformly distribute offset and size
            request.offset = rand() % SEGMENT_SIZE;
            request.size = (rand() % (SEGMENT_SIZE / 10)) + 1; // 1 to SEGMENT_SIZE/10 bytes
        } else if (strcmp(distribution, "hotspot") == 0) {
            // Concentrate requests on the first few segments
            request.offset = rand() % (SEGMENT_SIZE / 4);      // Focused on first quarter of segment
            request.size = (rand() % (SEGMENT_SIZE / 10)) + 1; // 1 to SEGMENT_SIZE/10 bytes
        } else if (strcmp(distribution, "sequential") == 0) {
            // Sequentially distribute offset and size
            request.offset = (i * (SEGMENT_SIZE / numRequests)) % SEGMENT_SIZE;
            request.size = (rand() % (SEGMENT_SIZE / 10)) + 1; // 1 to SEGMENT_SIZE/10 bytes
        } else {
            printf("Unknown distribution type.\n");
            return;
        }

        processWriteRequest(manager, &request);
    }
}

// Print workload summary
void printWorkloadSummary(SegmentManager *manager, const char *distribution) {
    uint64_t totalUtilization = 0;
    uint64_t totalSegmentsUsed = 0;

    for (int i = 0; i < NUM_SEGMENTS; i++) {
        if (manager->segments[i].utilization > 0) {
            totalUtilization += manager->segments[i].utilization;
            totalSegmentsUsed++;
        }
    }

    printf("\nWorkload Summary (%s):\n", distribution);
    printf("  Total Writes: %lu\n", manager->totalWrites);
    printf("  Total Invalidated Bytes: %lu\n", manager->totalInvalidated);
    printf("  Total Utilization: %lu bytes\n", totalUtilization);
    printf("  Total Segments Used: %lu/%d\n", totalSegmentsUsed, NUM_SEGMENTS);
}

int main() {
    SegmentManager manager;

    // Uniform Workload
    initSegmentManager(&manager);
    printf("\nGenerating uniform workload...\n");
    generateWorkload(&manager, 100, "uniform");
    printWorkloadSummary(&manager, "uniform");

    // Hotspot Workload
    initSegmentManager(&manager);
    printf("\nGenerating hotspot workload...\n");
    generateWorkload(&manager, 100, "hotspot");
    printWorkloadSummary(&manager, "hotspot");

    // Sequential Workload
    initSegmentManager(&manager);
    printf("\nGenerating sequential workload...\n");
    generateWorkload(&manager, 100, "sequential");
    printWorkloadSummary(&manager, "sequential");

    return 0;
}

