#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>

#include "generic.h"


// Test: createVector
void test_createVector() {
    Vector* v = createVector(0);
    assert(v == NULL);

    v = createVector(sizeof(int));
    assert(v != NULL);
    assert(v->size == 0);
    assert(v->capacity == 10);
    assert(v->elem_size == sizeof(int));
    assert(v->data != NULL);

    vectorFree(v);

    // Test huge elem_size (may fail gracefully)
    v = createVector(1000000000);
    if (v) vectorFree(v);
}

// Test: appendVectorItem + getVectorItem
void test_append_get() {
    Vector* v = createVector(sizeof(int));

    int values[] = {10, 20, 30, 40};
    for (int i = 0; i < 4; i++) {
        assert(appendVectorItem(v, &values[i]) == 0);
        assert(v->size == (size_t)(i + 1));
    }

    for (int i = 0; i < 4; i++) {
        int* item = (int*)getVectorItem(v, (size_t)i);
        assert(item != NULL);
        assert(*item == values[i]);
    }

    // Out of bounds
    assert(getVectorItem(v, 4) == NULL);
    assert(getVectorItem(v, 100) == NULL);

    vectorFree(v);
}

// Test: setVectorItem
void test_set() {
    Vector* v = createVector(sizeof(int));

    int a = 10, b = 999;
    appendVectorItem(v, &a);
    assert(setVectorItem(v, 0, &b) == 0);

    int* val = (int*)getVectorItem(v, 0);
    assert(val != NULL);
    assert(*val == 999);

    // Invalid index
    int c = 500;
    assert(setVectorItem(v, 1, &c) == -1); // out of bounds

    vectorFree(v);
}

// Test: popVectorItem
void test_pop() {
    Vector* v = createVector(sizeof(int));

    int values[] = {10, 20, 30};
    for (int i = 0; i < 3; i++) {
        appendVectorItem(v, &values[i]);
    }

    // Pop middle
    int* p = (int*)popVectorItem(v, 1);
    assert(p != NULL);
    assert(*p == 20);
    free(p);
    assert(v->size == 2);

    // Check remaining elements
    assert(*(int*)getVectorItem(v, 0) == 10);
    assert(*(int*)getVectorItem(v, 1) == 30);

    // Pop first
    p = (int*)popVectorItem(v, 0);
    assert(p != NULL);
    assert(*p == 10);
    free(p);
    assert(v->size == 1);

    // Pop last
    p = (int*)popVectorItem(v, 0);
    assert(p != NULL);
    assert(*p == 30);
    free(p);
    assert(v->size == 0);

    // Pop from empty
    assert(popVectorItem(v, 0) == NULL);

    vectorFree(v);
}

// Test: findVectorItem
void test_find() {
    Vector* v = createVector(sizeof(int));

    int values[] = {10, 20, 30, 20};
    for (int i = 0; i < 4; i++) {
        appendVectorItem(v, &values[i]);
    }

    int target = 20;
    long idx = findVectorItem(v, &target, intEquals);
    assert(idx == 1); // first occurrence

    idx = findVectorItem(v, &target, NULL); // use memcmp
    assert(idx == 1);

    int not_found = 999;
    idx = findVectorItem(v, &not_found, intEquals);
    assert(idx == -1);

    // NULL vector
    idx = findVectorItem(NULL, &target, intEquals);
    assert(idx == -2);

    vectorFree(v);
}

// Test: auto-resize (grow and shrink)
void test_resize() {
    Vector* v = createVector(sizeof(int));

    // Fill beyond initial capacity (10)
    for (int i = 0; i < 15; i++) {
        int val = i;
        assert(appendVectorItem(v, &val) == 0);
    }
    assert(v->capacity >= 15); // should have grown

    // Now pop until shrink threshold (~25%)
    while (v->size > 2) { // 2 < 10/4 → should trigger shrink
        void* p = popVectorItem(v, 0);
        assert(p != NULL);
        free(p);
    }
    // Capacity may or may not shrink — depends on resize policy
    assert(v->size == 2);

    vectorFree(v);
}

// Test: vectorFree and edge cases
void test_free_edge() {
    // Free NULL
    assert(vectorFree(NULL) == -1);

    // Create, do nothing, free
    Vector* v = createVector(sizeof(int));
    assert(v != NULL);
    assert(vectorFree(v) == 0);

    // Create, add one, free
    v = createVector(sizeof(char));
    char c = 'A';
    appendVectorItem(v, &c);
    assert(vectorFree(v) == 0);
}

///////////////////////////////////////////////////////////////////////////////
// STRESS TESTS
///////////////////////////////////////////////////////////////////////////////

// Stress Test 1: Append 100_000_000 integers
void stress_test_large_append() {
    const size_t N = 100000000;
    Vector* v = createVector(sizeof(int));
    assert(v != NULL);

    for (size_t i = 0; i < N; i++) {
        int val = (int)i;
        assert(appendVectorItem(v, &val) == 0);
        if (i % 10000 == 0) {
            assert(v->size == i + 1);
        }
    }

    assert(v->size == N);
    assert(*(int*)getVectorItem(v, 0) == 0);
    assert(*(int*)getVectorItem(v, N-1) == (int)(N-1));

    vectorFree(v);
}

// Stress Test 2: Append + Pop Back 5_000_000 times
void stress_test_append_pop_back() {
    const size_t N = 5000000;
    Vector* v = createVector(sizeof(int));
    assert(v != NULL);

    for (size_t i = 0; i < N; i++) {
        int val = (int)i;
        assert(appendVectorItem(v, &val) == 0);
    }

    for (size_t i = 0; i < N; i++) {
        void* p = popVectorItem(v, v->size - 1); // pop back
        assert(p != NULL);
        int popped = *(int*)p;
        assert(popped == (int)(N - 1 - i));
        free(p);
    }

    assert(v->size == 0);
    vectorFree(v);
}

// Stress Test 3: Random insert/remove in middle (simplified: random pop)
void stress_test_random_access() {
    const size_t N = 10000;
    Vector* v = createVector(sizeof(int));
    assert(v != NULL);

    // Fill vector
    for (size_t i = 0; i < N; i++) {
        int val = (int)i;
        assert(appendVectorItem(v, &val) == 0);
    }

    srand((unsigned int)time(NULL));

    // Randomly remove 50% of elements
    for (size_t i = 0; i < N / 2; i++) {
        if (v->size == 0) break;
        size_t rand_index = rand() % v->size;
        void* p = popVectorItem(v, rand_index);
        assert(p != NULL);
        free(p);
    }

    // Remaining elements should be consistent
    assert(v->size == N / 2);

    vectorFree(v);
}

// Stress Test 4: Mixed types — store doubles and ints alternately
void stress_test_mixed_types() {
    const size_t N = 10000;
    Vector* v = createVector(sizeof(double)); // larger type
    assert(v != NULL);

    for (size_t i = 0; i < N; i++) {
        if (i % 2 == 0) {
            int val = (int)(i * 2);
            assert(appendVectorItem(v, &val) == 0);
        } else {
            double val = i * 3.14159;
            assert(appendVectorItem(v, &val) == 0);
        }
    }

    assert(v->size == N);

    // Just remove everything — no validation, just stability
    while (v->size > 0) {
        void* p = popVectorItem(v, 0);
        assert(p != NULL);
        free(p);
    }

    vectorFree(v);
}

// Stress Test 5: Push to limit, then shrink aggressively
void stress_test_shrink_to_min() {
    Vector* v = createVector(sizeof(int));
    assert(v != NULL);

    // Grow to large capacity
    for (int i = 0; i < 100; i++) {
        int val = i;
        assert(appendVectorItem(v, &val) == 0);
    }

    // Shrink by popping all but 1
    while (v->size > 1) {
        void* p = popVectorItem(v, 0);
        assert(p != NULL);
        free(p);
    }

    // Final size = 1, capacity should be >= MIN_CAPACITY (4)
    assert(v->size == 1);
    assert(v->capacity >= 4);

    vectorFree(v);
}

int main() {
    printf("Running functional and edge case tests...\n");

    test_createVector();
    test_append_get();
    test_set();
    test_pop();
    test_find();
    test_resize();
    test_free_edge();

    printf("Running stress tests (may take a few seconds)...\n");

    stress_test_large_append();
    stress_test_append_pop_back();
    stress_test_random_access();
    stress_test_mixed_types();
    stress_test_shrink_to_min();

    printf("All tests passed successfully.\n");
    return 0;
}