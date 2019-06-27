#ifndef __CELLTASK_H_H
#define __CELLTASK_H_H
#include<thread>
#include<mutex>
#include<list>

//�������
class CellTask
{
public:
	CellTask()
	{

	}
	//����������
	virtual ~CellTask()
	{

	}
	//ִ�����񷽷�
	virtual void doTask()
	{

	}
private:

};

typedef std::shared_ptr<CellTask> CellTaskPtr;
//ִ������ķ�����
class CellTaskServer
{
public:
	CellTaskServer()
	{

	}
	~CellTaskServer()
	{

	}
	//�������
	void addTask(CellTaskPtr& task)
	{
		std::lock_guard<std::mutex> _lock_guard(_mutex);
		_taskBuff.push_back(task);
	}
	//��������
	void Start()
	{
		//�߳�
		std::thread _thread(std::mem_fn(&CellTaskServer::onRun),this);
		_thread.detach();
	}
private:
	//�̳߳���ִ�еĹ�������
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
			//���û������
			if (_tasks.empty())
			{
				std::chrono::milliseconds t(1);
				std::this_thread::sleep_for(t);
				continue;
			}
			else //��������
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
	//��������
	std::list<CellTaskPtr> _tasks;
	//������ݵĻ�����
	std::list<CellTaskPtr> _taskBuff;
	//������  �ı����ݻ�����ʱ ��Ҫ����
	std::mutex _mutex;

};
#endif // !__CELLTASK_H_H
