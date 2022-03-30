#ifndef LIST_H
#define LIST_H

#ifdef __cplusplus
extern "C" {
#endif


	/*
	 * Allocates a new list and assign the object to list
	 */
	void vector_alloc(void** list);
	/*
	 * Free's any memory associated with list. NOTE: this function
	 * does not free the any items. You must pass an empty list
	 * to this function or all your objects will leak!
	 */
	void vector_destroy(void* list);

	/*
	 * Adds a new object to the end of a given list
	 */
	void vector_add(void* list, void* job);

	/*
	 * Removes the object at the end of the list and
	 * returns it.
	 */
	void* vector_remove(void* list);

	/*
	 * Returns the size of the list
	 */
	int vector_size(void* list);

#ifdef __cplusplus
}
#endif


#endif
