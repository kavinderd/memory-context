/*
 * Part 2
 * An toy implementation of PostgreSQL/HAWQ's paradigm of Memory Contexts
 * Compile: cc -g -o memory-context memory-context.c
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>

#define MAX_MEMORY_CHUNKS 10
#define MEMORY_CHUNK_HEADER sizeof(MemoryChunk)

struct MemoryContext;

typedef struct MemoryChunk {
	int size;
	struct MemoryChunk* next;
	struct MemoryContext* context;
	void* ptr_start;
} MemoryChunk;

typedef struct MemoryContext {
	int free_index;
	MemoryChunk* chunks; // The head of the list of free memory chunks
	MemoryChunk* free_chunks[MAX_MEMORY_CHUNKS]; // List of free chunks of memory
} MemoryContext;

MemoryContext* currentContext;

/*
 * Allocate the chunk of memory to the context
 * set the remaining size to account for the 
 * three int values.
 */
MemoryContext* generateMemoryContext() {
	MemoryContext* context = malloc(sizeof(MemoryContext));
	context->free_index = -1;
	int i;
	for (int i = 0; i < MAX_MEMORY_CHUNKS; i++)
		context->free_chunks[i] = NULL;
	context->chunks = NULL;
	return context;
}

/* 
 * Free all the chunks belonging to a context
 */

void freeChunks(MemoryChunk* chunk) {
	MemoryChunk *current, *next;
	current = chunk;
	do {
		next = current->next;
		free(current);
		current = next;
	} while (current);
}

/*
 * To delete a context simply call free()
 * Since the context is the entire chunk
 * everything allocated through `mallocate()`
 * will also be freed
 */
void deleteContext(MemoryContext* context) {
	if (context->chunks) 
		freeChunks(context->chunks);

	free(context);
	context = NULL;
}

/*
 * Set the global variable currentContext to 
 * the parameter
 */
void setCurrentContext(MemoryContext* context) {
	currentContext = context;
}

MemoryChunk* createChunk(int size) {
	MemoryChunk* chunk;
	chunk = malloc(MEMORY_CHUNK_HEADER + size); // Allocate the size requested plus the size of a Memory Chunk
	chunk->context = currentContext; //Set the chunks context to the current context
	chunk->size = size; // Set the size for bookeeping
	chunk->next = NULL; // Explicitly set next pointer to NULL
	chunk->ptr_start = ((char*)chunk + MEMORY_CHUNK_HEADER); // The pointer we return will come after the memory taken by the memory chunk itself
	return chunk;
}

/*
 * Take the address of the context
 * and set a pointer to void at the 
 * offset of the earliest non-used portion 
 * of memory
 */
void* mallocate(int size) {
	MemoryChunk* chunk;
	if (currentContext->free_index >= 0 && currentContext->free_chunks[currentContext->free_index] && currentContext->free_chunks[currentContext->free_index]->size >= size) {
		//If we have a chunk in the free_list and its size is >= what was requested use it
		chunk = currentContext->free_chunks[currentContext->free_index--];
	} else {
		//Create a new chunk
		chunk = createChunk(size);
		if (currentContext->chunks ) {
			//Add chunk to existing list of the currentContext
			chunk->next = currentContext->chunks;
			currentContext->chunks = chunk;
		} else {
			//Set this chunk as the first of the currentContext
			currentContext->chunks = chunk;
		}
	}
	//Return the pointer that is a member of the MemoryChunk
	return chunk->ptr_start;
}

/*
 * Free mallocated memory
 * does not release memory back to OS, but simply adds it to
 * an internal list in the Memory Context
 */
void befree(void* pointer) {
	currentContext->free_index++; //Increment free_index of the currentContext to add chunk to next available location
	MemoryChunk* chunk = (MemoryChunk*)(((char*)pointer) - MEMORY_CHUNK_HEADER); // Given the pointer, calculate the address of the parent MemoryChunk
	currentContext->free_chunks[currentContext->free_index] = chunk; //Add the chunk to the free_list of the currentContext
}

int main() {
	MemoryContext* context = generateMemoryContext();
	setCurrentContext(context);

	char* string1 = mallocate(32);
	strcpy(string1, "Test String 1");
	printf("String 1: %s\n", string1);

	char* string2 = mallocate(32);
	strcpy(string2, "Test String 2");
	printf("String 2: %s\n", string2);

	befree(string2);

	char* string3 = mallocate(32);
	strcpy(string3, "Not Test String 2");
	printf("String 3: %s\n", string3);
	printf("String 2: %s\n", string2);

	deleteContext(context);
}
