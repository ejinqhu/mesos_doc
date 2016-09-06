#include <iostream>
#include <mesos/executor.hpp>
#include <stout/duration.hpp>
#include <stout/os.hpp>

using namespace mesos;
using namespace std;

class MyExecutor : public Executor
{
public:
    MyExecutor()
    {
        cout << "MyExecutor created." << endl;
    }
    virtual ~MyExecutor() 
    {
        cout << "MyExecutor destoryed." << endl;
    }

    virtual void registered(ExecutorDriver* driver, const ExecutorInfo& executorInfo, 
                            const FrameworkInfo& frameworkInfo, const SlaveInfo& slaveInfo)
    {
        cout << "Registered executor on " << slaveInfo.hostname() << endl;
    }

    virtual void reregistered(ExecutorDriver* driver, const SlaveInfo& slaveInfo)
    {
        cout << "Reregistered executor on " << slaveInfo.hostname() << endl;
    }

    virtual void disconnected(ExecutorDriver* driver) {}
    virtual void killTask(ExecutorDriver* driver, const TaskID& taskId) {}
    virtual void frameworkMessage(ExecutorDriver* driver, const string& data) {}
    virtual void shutdown(ExecutorDriver* driver) {}
    virtual void error(ExecutorDriver* driver, const string& message) {}
    virtual void launchTask(ExecutorDriver* driver, const TaskInfo& task);
};


void MyExecutor::launchTask(ExecutorDriver* driver, const TaskInfo& task)
{
    cout << "Start task " << task.task_id().value() << endl;

    TaskStatus status;
    status.mutable_task_id()->MergeFrom(task.task_id());
    status.set_state(TASK_RUNNING);

    driver->sendStatusUpdate(status);

    // here we may do real work

    cout << "Finish task " << task.task_id().value() << endl;

    status.mutable_task_id()->MergeFrom(task.task_id());
    status.set_state(TASK_FINISHED);

    driver->sendStatusUpdate(status);

    return;
}


int main(int argc, char** argv)
{
    MyExecutor executor;
    MesosExecutorDriver driver(&executor);
    return (driver.run() == DRIVER_STOPPED) ? 0 : 1;
}
