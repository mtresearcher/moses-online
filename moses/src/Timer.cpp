/*
 *  Timer.cpp
 *  moses
 *
 *  Created by Hieu Hoang on 28/06/2010.
 *  Copyright 2010 __MyCompanyName__. All rights reserved.
 *
 */

#include "Timer.h"

Timer Timer::s_instance;

void Timer::ResetUserTime()
{
  s_instance.start();
}

void Timer::PrintUserTime(const std::string &message)
{ 
	s_instance.check(message.c_str());
}
