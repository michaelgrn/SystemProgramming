#include <vector>
#include <iostream>
#include "list.h"

using namespace std;


void vector_alloc(void** list)
{
	*list = new vector<void*>();
}

void vector_destroy(void* list)
{
	//Yep cast from void* to vector. C land will hold onto the
	//reference as a void* but when we need to use it we have to
	//convert it back. Yes this is very very dangerous, but when
	//you interact with C most stuff is :)
	vector<void*>* temp = reinterpret_cast<vector<void*>*>(list);
	//delete the vector. This implementation assumes that you are
	//passing it an empty vector because we really have no way of
	//freeing the objects inside. If we wanted the destroy function
	//to also free any remaining elements we would need to pass in
	//a function pointer to a free function.
	delete temp;

}

void vector_add(void* list, void* object)
{
	vector<void*>* temp = reinterpret_cast<vector<void*>*>(list);
	temp->push_back(object);
}

void* vector_remove(void* list)
{
	vector<void*>* temp = reinterpret_cast<vector<void*>*>(list);
	void* rval = 0;
	if(!temp->empty()){
		rval=temp->back();
		temp->pop_back();
	}
	return rval;
}

int vector_size(void* list)
{
	vector<void*>* temp = reinterpret_cast<vector<void*>*>(list);
	return temp->size();
}
