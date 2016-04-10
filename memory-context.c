/*
 * An toy implementation of PostgreSQL/HAWQ's paradigm of Memory Contexts
 * Compile: cc -g -o memory-context memory-context.c
 */
#include <stdio.h>
#include <stdlib.h>

#define NO_CONTEXT_SET 1


typedef struct MemoryContext {
	int size;           // The total amount of memory to allocate to the context
	int state;          // The state the context is in.
	int remaining_size; // The amount of free memory the context has
} MemoryContext;

MemoryContext* currentContext;

/*
 * Allocate the chunk of memory to the context
 * set the remaining size to account for the 
 * three int values.
 */
MemoryContext* generateMemoryContext(int size) {
	MemoryContext* context = malloc(size);
	context->size = size;
	context->remaining_size = size - (3 * sizeof(int));
	return context;
}

/*
 * To delete a context simply call free()
 * Since the context is the entire chunk
 * everything allocated through `mallocate()`
 * will also be freed
 */
void deleteContext(MemoryContext* context) {
	free(context);
	context = NULL;
}

/*
 * Set the global variable currentContext to 
 * the parameter
 */
void setCurrentContext(MemoryContext* context) {
	printf("Setting Current Context\n");
	currentContext = context;
}

/*
 * Take the address of the context
 * and set a pointer to void at the 
 * offset of the earliest non-used portion 
 * of memory
 */
void* mallocate(int size) {
	void* ptr = currentContext + currentContext->size - currentContext->remaining_size;
	currentContext->remaining_size -= size;
	return ptr;
}

int main() {
	MemoryContext* smallContext = generateMemoryContext(256);
	MemoryContext* bigContext = generateMemoryContext(512);
	setCurrentContext(smallContext);

	char* stringInSmall = mallocate(32);
	stringInSmall = "A Small String";
	printf("String In Small: %s\n", stringInSmall);

	char* anotherInSmall = mallocate(16);
	anotherInSmall = "A Second String";
	printf("String In Small: %s\n", anotherInSmall);

	setCurrentContext(bigContext);

	char* stringInBigContext = mallocate(64);
	stringInBigContext = "A Much Bigger String";
	printf("String In Big: %s\n", stringInBigContext);
	deleteContext(smallContext);
	deleteContext(bigContext);
}
