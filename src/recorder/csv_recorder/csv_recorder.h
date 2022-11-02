// recorder.h: 目标的头文件。

#pragma once
#include <recorder.h>

class csv_recorder : public recorder
{ 
	virtual void on_trigger(event_type type);

};