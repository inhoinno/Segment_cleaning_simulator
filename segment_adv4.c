#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>

#define SEGMENT_SIZE 1024  // Size of each segment in bytes
#define NUM_SEGMENTS 1024  // Total number of segments
#define GC_THRESHOLD 0.9   // Threshold to trigger garbage collection (90% utilization)

// Define infinity symbol for GC cost
#define INFINITY_SYMBOL "∞"

typedef struct {
    int segmentId;
    uint64_t utilization;          // Current utilization of the segment
    uint8_t data[SEGMENT_SIZE];    // Simulated storage space
    int valid[SEGMENT_SIZE];       // Validity map for each byte (1 = valid, 0 = invalid)
    uint64_t invalidatedBytes;     // Tracks invalidated bytes for GC
} Segment;

typedef struct {
    Segment segments[NUM_SEGMENTS];
    uint64_t totalWrites;          // Total write requests processed
    uint64_t totalInvalidated;     // Total invalidated bytes across all segments
    uint64_t gcCount;              // Total garbage collection processes
    double totalGcCost;            // Accumulated GC cost
} SegmentManager;

typedef struct {
    uint64_t offset;  // Logical Block Address (LBA)
    uint64_t size;    // Size of the write in bytes
} WriteRequest;
// Function Declarations
void initSegmentManager(SegmentManager *manager);
Segment *findSegmentForWrite(SegmentManager *manager, uint64_t size);
void invalidateOldData(Segment *segment, uint64_t offset, uint64_t size, SegmentManager *manager);
void writeToSegment(Segment *segment, uint64_t offset, uint64_t size, SegmentManager *manager);
void processWriteRequest(SegmentManager *manager, WriteRequest *request);
int isGarbageCollectionNeeded(SegmentManager *manager);
void garbageCollect(SegmentManager *manager);
void generateWorkload(SegmentManager *manager, int numRequests, const char *distribution);
void addMoreWorkloads(SegmentManager *manager, int numRequests);
void printSegmentDetails(SegmentManager *manager);

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
    manager->gcCount = 0;
    manager->totalGcCost = 0.0;
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
        printf("Warning: Space full, triggering garbage collection...\n");
        garbageCollect(manager);
        targetSegment = findSegmentForWrite(manager, request->size);
        if (targetSegment == NULL) {
            printf("Error: No space available after garbage collection (offset: %lu, size: %lu)\n",
                   request->offset, request->size);
            return;
        }
    }

    // Invalidate old data if it is an overwrite request
    invalidateOldData(targetSegment, request->offset, request->size, manager);

    // Write new data
    writeToSegment(targetSegment, request->offset, request->size, manager);

    manager->totalWrites++;
}

// Check if garbage collection is needed
int isGarbageCollectionNeeded(SegmentManager *manager) {
    int usedSegments = 0;
    for (int i = 0; i < NUM_SEGMENTS; i++) {
        if (manager->segments[i].utilization > 0) {
            usedSegments++;
        }
    }
    return ((double)usedSegments / NUM_SEGMENTS) >= GC_THRESHOLD;
}

// Perform garbage collection
void garbageCollect(SegmentManager *manager) {
    int victimIndex = -1;
    uint64_t maxInvalidated = 0;

    // Find the victim segment with the most invalidated bytes and lowest utilization
    for (int i = 0; i < NUM_SEGMENTS; i++) {
        Segment *segment = &manager->segments[i];
        if (segment->invalidatedBytes > maxInvalidated && segment->utilization > 0) {
            maxInvalidated = segment->invalidatedBytes;
            victimIndex = i;
        }
    }

    if (victimIndex == -1) {
        printf("Garbage Collection: No suitable victim found.\n");
        return;
    }

    Segment *victim = &manager->segments[victimIndex];
    Segment *newSegment = findSegmentForWrite(manager, 0);

    if (newSegment == NULL) {
        printf("Garbage Collection: No space available for compaction.\n");
        return;
    }

    // Copy valid data from the victim to a new segment
    for (int i = 0; i < SEGMENT_SIZE; i++) {
        if (victim->valid[i]) {
            writeToSegment(newSegment, i, 1, manager); // Copy one byte at a time
            victim->valid[i] = 0;
        }
    }

    // Reset victim segment
    victim->utilization = 0;
    victim->invalidatedBytes = 0;
    memset(victim->data, 0, SEGMENT_SIZE);

    // Calculate and print GC cost
    double utilization = (double)manager->totalWrites / (NUM_SEGMENTS * SEGMENT_SIZE);
    printf("Garbage Collection: Freed Segment %d\n", victim->segmentId);
    if (utilization < 1.0) {
        double gcCost = 2.0 / (1.0 - utilization);
        manager->totalGcCost += gcCost;
        printf("Garbage Collection Cost: %.2f\n", gcCost);
    } else {
        printf("Garbage Collection Cost: %s\n", INFINITY_SYMBOL);
    }

    manager->gcCount++;
}

// Add more workloads
void addMoreWorkloads(SegmentManager *manager, int numRequests) {
    printf("\nAdding more workloads...\n");
    generateWorkload(manager, numRequests, "uniform");
}

// Workload Generator
void generateWorkload(SegmentManager *manager, int numRequests, const char *distribution) {
    WriteRequest request;
    srand(time(NULL));

    for (int i = 0; i < numRequests; i++) {
        request.offset = rand() % SEGMENT_SIZE;
        request.size = (rand() % (SEGMENT_SIZE / 10)) + 1; // 1 to SEGMENT_SIZE/10 bytes
        processWriteRequest(manager, &request);

        // Trigger garbage collection if needed
        if (isGarbageCollectionNeeded(manager)) {
            garbageCollect(manager);
        }
    }
}

// Print segment details
void printSegmentDetails(SegmentManager *manager) {
    for (int i = 0; i < NUM_SEGMENTS; i++) {
        Segment *segment = &manager->segments[i];
        if (segment->utilization > 0) {
            printf("Segment ID: %d, Utilization: %lu/%d, Invalidated Bytes: %lu\n", 
                   segment->segmentId, segment->utilization, SEGMENT_SIZE, segment->invalidatedBytes);
        }
    }
}

int main() {
    SegmentManager manager;
    initSegmentManager(&manager);

    printf("\nGenerating workload...\n");
    generateWorkload(&manager, 2000, "uniform");

    printf("\nAdding more workloads...\n");
    addMoreWorkloads(&manager, 1000);

    printf("\nSegment Details After Workloads:\n");
    printSegmentDetails(&manager);

    return 0;
}

