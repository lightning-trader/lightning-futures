#pragma once
#include <define.h>

extern "C"
{
	EXPORT_FLAG void* maping_file(const char * path,size_t size);
	
	EXPORT_FLAG void unmaping_file(void* dataptr, size_t size);

}

template<typename T>
T* maping_file(const char* path)
{
	return static_cast<T*>(maping_file(path,sizeof(T)));
}
template<typename T>
void unmaping_file(T*& dataptr)
{
	unmaping_file((dataptr),sizeof(T));
	dataptr = nullptr;
}
