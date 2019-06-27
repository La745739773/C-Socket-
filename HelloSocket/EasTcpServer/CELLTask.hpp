#ifndef __CELLTASK_H_H
#define __CELLTASK_H_H
#include<thread>
#include<mutex>
#include<list>

//任务基类
class CellTask
{
public:
	CellTask()
	{

	}
	//虚析构函数
	virtual ~CellTask()
	{

	}
	//执行任务方法
	virtual void doTask()
	{

	}
private:

};

typedef std::shared_ptr<CellTask> CellTaskPtr;
//执行任务的服务类
class CellTaskServer
{
public:
	CellTaskServer()
	{

	}
	~CellTaskServer()
	{

	}
	//添加任务
	void addTask(CellTaskPtr& task)
	{
		std::lock_guard<std::mutex> _lock_guard(_mutex);
		_taskBuff.push_back(task);
	}
	//启动服务
	void Start()
	{
		//线程
		std::thread _thread(std::mem_fn(&CellTaskServer::onRun),this);
		_thread.detach();
	}
private:
	//线程持续执行的工作函数
	void onRun()
	{
		while (true)
		{
			if (!_taskBuff.empty())
			{
				std::lock_guard<std::mutex> _lock_guard(_mutex);
				for (auto task : _taskBuff)
				{
					_tasks.push_back(task);
				}
				_taskBuff.clear();
			}
			//如果没有任务
			if (_tasks.empty())
			{
				std::chrono::milliseconds t(1);
				std::this_thread::sleep_for(t);
				continue;
			}
			else //处理任务
			{
				for (auto task : _tasks)
				{
					task->doTask();
				}
			}
			_tasks.clear();
		}
	}
private:
	//任务数据
	std::list<CellTaskPtr> _tasks;
	//添加数据的缓冲区
	std::list<CellTaskPtr> _taskBuff;
	//互斥锁  改变数据缓冲区时 需要加锁
	std::mutex _mutex;

};
#endif // !__CELLTASK_H_H
