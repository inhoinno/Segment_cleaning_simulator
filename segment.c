#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define SEGMENT_SIZE 1024  // Size of each segment in bytes
#define NUM_SEGMENTS 1024     // Total number of segments

typedef struct {
    int segmentId;
    uint64_t utilization;          // Current utilization of the segment
    uint8_t page[SEGMENT_SIZE];    // Simulated storage space
    int valid[SEGMENT_SIZE];       // Validity map for each byte (1 = valid, 0 = invalid)
} Segment;

typedef struct {
    Segment segments[NUM_SEGMENTS];
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
        memset(manager->segments[i].page, 0, SEGMENT_SIZE);
        memset(manager->segments[i].valid, 0, SEGMENT_SIZE);
    }
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
void invalidateOldData(Segment *segment, uint64_t offset, uint64_t size) {
    for (uint64_t i = offset; i < offset + size; i++) {
        if (i < SEGMENT_SIZE && segment->valid[i]) {
            segment->valid[i] = 0;
            segment->utilization--;
        }
    }
}

// Write data to the segment
void writeToSegment(Segment *segment, uint64_t offset, uint64_t size) {
    for (uint64_t i = offset; i < offset + size; i++) {
        if (i < SEGMENT_SIZE) {
            segment->page[i] = 1;  // Simulated data write
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
    invalidateOldData(targetSegment, request->offset, request->size);

    // Write new data
    writeToSegment(targetSegment, request->offset, request->size);

    printf("Write request processed: Segment ID %d, Offset %lu, Size %lu\n",
           targetSegment->segmentId, request->offset, request->size);
}

// Print segment details
void printSegmentDetails(SegmentManager *manager) {
    for (int i = 0; i < NUM_SEGMENTS; i++) {
        Segment *segment = &manager->segments[i];
        printf("Segment ID: %d, Utilization: %lu/%d\n", 
               segment->segmentId, segment->utilization, SEGMENT_SIZE);
    }
}

int main() {
    SegmentManager manager;
    initSegmentManager(&manager);

    WriteRequest requests[] = {
        {0, 100},   // Write 100 bytes at offset 0
        {50, 50},   // Overwrite 50 bytes starting at offset 50
        {200, 300}, // Write 300 bytes at offset 200
        {300, 100},  // Overwrite the entire segment
        {400, 100},
        {900,100}
    };

    int numRequests = sizeof(requests) / sizeof(requests[0]);

    for (int i = 0; i < numRequests; i++) {
        processWriteRequest(&manager, &requests[i]);
    }

    printSegmentDetails(&manager);

    return 0;
}

