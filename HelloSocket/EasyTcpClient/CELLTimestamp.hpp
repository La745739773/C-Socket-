#ifndef CELLTimestamp_HPP_
#define CELLTimestamp_HPP_

#include<chrono>
using namespace std::chrono;
class CELLTimestamp
{
public:
	CELLTimestamp()
	{
		update();
	}
	~CELLTimestamp()
	{

	}
	double getElapsedSecond()
	{
		return this->getElapsedTimeInMicroSec() * 0.001 * 0.001;
	}
	// ∫¡√Î
	long long getElapsedTimeInMilliSec() 
	{
		return this->getElapsedTimeInMicroSec() * 0.001;
	}
	//ªÒ»°Œ¢√Î
	long long getElapsedTimeInMicroSec()
	{
		return duration_cast<microseconds>(high_resolution_clock::now() - _begin).count();
	}
	void update()
	{
		_begin = high_resolution_clock::now();
	}

protected:
	time_point<high_resolution_clock>  _begin;
};
#endif