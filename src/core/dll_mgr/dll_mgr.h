#pragma once
#include <string>

class dll_mgr
{
	/*
	* ╪сть dll дё©И
	*/
	virtual bool load(const std::string& file_name) = 0;

	/*
	 *	п╤ть
	 */
	virtual void unload() = 0;
};