#include <iostream>
#include <string>
#include <boost/lexical_cast.hpp>
#include <mesos/resources.hpp>
#include <mesos/scheduler.hpp>
#include <mesos/type_utils.hpp>
#include <stout/os.hpp>
#include <stout/path.hpp>

using namespace std;
using namespace mesos;
using boost::lexical_cast;
using mesos::Resources;
using mesos::CommandInfo;

const int TOTAL_TASK_NUM  = 9;
const int SLAVE_TASK_NUM  = 3;
const int CPUS_PER_TASK   = 1;
const int MEM_PER_TASK    = 32;


class MyScheduler : public Scheduler
{
public:
    MyScheduler( bool _implicitAck, const ExecutorInfo& _executor, const string& _role)
    {
        implicitAck   = _implicitAck;
        executor      = _executor;
        role          = _role;
        tasksLaunched = 0;
        tasksFinished = 0;
        totalTasks    = TOTAL_TASK_NUM;
        master        = "";
        exeurl        = "";
        useLocalExeFile = true;
        totalTasksPerSlave = SLAVE_TASK_NUM;
    }

    virtual ~MyScheduler() 
    {
        cout << "Scheduler destoryed." << endl;
    }


    virtual void registered(SchedulerDriver*, const FrameworkID&, const MasterInfo&)
    {
        cout << "Registered into mesos master." << endl;
    }

    virtual void reregistered(SchedulerDriver*, const MasterInfo& masterInfo)
    {
        cout << "Master changed, ReRegistered into it."  << endl;
    }

    virtual void disconnected(SchedulerDriver* driver) { }
    virtual void offerRescinded(SchedulerDriver* driver, const OfferID& offerId) { } 
    virtual void frameworkMessage(SchedulerDriver* driver, const ExecutorID& executorId, 
                                  const SlaveID& slaveId, const string& data) { }
    virtual void slaveLost(SchedulerDriver* driver, const SlaveID& sid) { }
    virtual void executorLost(SchedulerDriver* driver, const ExecutorID& executorID, 
                              const SlaveID& slaveID, int status) { }
    virtual void error(SchedulerDriver* driver, const string& errMsg) 
    {   
        cout << errMsg << endl; 
    }

    // Implement later in another function.
    virtual void resourceOffers(SchedulerDriver* driver, const vector<Offer>& offers);
    virtual void statusUpdate(SchedulerDriver* driver, const TaskStatus& status);


public:
    string       master;
    string       exeurl;  // fetch url via http/ftp/hdfs, etc.
    bool         useLocalExeFile;

private:
    bool implicitAck;
    int  tasksLaunched;
    int  tasksFinished;
    int  totalTasksPerSlave;
    int  totalTasks;
    string       role;
    ExecutorInfo executor;
};


// when task finished or abnormal status, this function will be called for future handling.
void MyScheduler::statusUpdate(SchedulerDriver* driver, const TaskStatus& status)
{
    int taskId = lexical_cast<int>(status.task_id().value());

    cout << "Task " << taskId << " is in state " << status.state() << endl;

    if (status.state() == TASK_FINISHED)
    {
        ++tasksFinished;
    }

    if (status.state() == TASK_LOST || status.state() == TASK_KILLED || status.state() == TASK_FAILED)
    {
        cout << "task " << taskId << " in abnoraml state " << status.state() << " aborted"  
             << " with reason " << status.reason() << " from source " << status.source() << endl;
        driver->abort();
    }

    // all completed, we may stop driver now.
    if (tasksFinished == totalTasks)
    {
        cout << "All task completed, total num:" << tasksFinished << endl;
        driver->stop();
    }


    // here if implicitAck set to false, we need to send ACK back to executor (slave).
    // like this:
    // driver->acknowledgeStatusUpdate(status);
    // this function is best effort, not reliable. So DON'T DEPEND ON IT in real product env.
    //

    return;
}


// when recv offer, we may check if we can start corresponding task.
void MyScheduler::resourceOffers(SchedulerDriver* driver, const vector<Offer>& offers)
{
    static int index  = 0;
    static int taskId = 0;

    

    cout << "recv offers number:" << offers.size() << endl;
    foreach (const Offer& offer, offers)
    {
        cout << "Index: " << index <<" recv offer " << offer.id() << " with " << offer.resources() << endl;
        static const Resources TASK_RESOURCES = Resources::parse("cpus:" + stringify(CPUS_PER_TASK) + 
                                                                 "; mem:" + stringify(MEM_PER_TASK)).get();
        Resources remaining = offer.resources();

        // Launch tasks.
        tasksLaunched = 0;
        vector<TaskInfo> tasks;
        tasks.clear();
        while (tasksLaunched < totalTasksPerSlave )
        {
            taskId++;
            tasksLaunched++;

            cout << "Begin to start task " << taskId << " using offer " << offer.id() << endl;

            TaskInfo task;
            task.set_name("Task " + lexical_cast<string>(taskId));
            task.mutable_task_id()->set_value(lexical_cast<string>(taskId));
            task.mutable_slave_id()->MergeFrom(offer.slave_id());

            // here, executor/command ONLY set one, don't set both, otherwise, error report.
            if ( useLocalExeFile == true)
            {
                //cout << "use local file" << endl;
            	task.mutable_executor()->MergeFrom(executor);
            }
            else
            {
                //cout << "use http file" << endl;
                CommandInfo commandInfo;
                CommandInfo::URI* uri = commandInfo.add_uris();
                uri->set_value(exeurl);
                commandInfo.set_value("true"); // Required to specify a value. 
            	task.mutable_command()->CopyFrom(commandInfo);
            }
             

            Option<Resources> resources = remaining.find(TASK_RESOURCES.flatten(role));

            CHECK_SOME(resources);
            task.mutable_resources()->MergeFrom(resources.get());
            remaining -= resources.get();

            tasks.push_back(task);
        }

        cout << "---- Launch task on " << offer.id() << endl;
        driver->launchTasks(offer.id(), tasks);
        index++;
    }

    return;
}


int main(int argc, char** argv)
{
    if ( argc != 3 && argc != 5)
    {
        cerr << "Usage: \n"
             << "./my_framework -master masterip:port  \n"
             << "or: \n"
             << "./my_framework -master masterip:port -uri uri_addr \n" << endl;
        exit(1);
    }

    int  status;
    bool implicitAck;
    bool useLocalExeFile = true;;
    std::string          uri;
    std::string          master;
    std::string          exeurl;
    ExecutorInfo         executor;
    FrameworkInfo        framework;
    MesosSchedulerDriver *driver;

    // find executor path
    if ( argc == 3) // find excutor in directory
    {
    	uri = path::join(os::realpath(Path(argv[0]).dirname()).get(),"my_executor");
    }
    else if ( argc == 5) // find executor via protocals (http, ftp, hdfs, etc)
    {
        uri = argv[4];
        exeurl = argv[4];
        useLocalExeFile = false;
    }

    master = argv[2];  // master ip

    cout << "master:" << master << ", uri:" << uri << ", exeurl:" << exeurl << endl;

    executor.mutable_executor_id()->set_value("executor");
    executor.mutable_command()->set_value(uri);
    executor.set_name("My Executor");
    executor.set_source("mytest");

    framework.set_user(""); 
    framework.set_name("My Framework");
    framework.set_role("role");

    implicitAck= true;

    MyScheduler scheduler(implicitAck, executor, "role");
 
    scheduler.master = master;
    scheduler.exeurl = exeurl; 
    scheduler.useLocalExeFile = useLocalExeFile;
    
    //cout << "++++ scheduler master: " << scheduler.master << ", exeurl:" << scheduler.exeurl << ", uselocal:" << scheduler.useLocalExeFile << endl;

    driver = new MesosSchedulerDriver(&scheduler, framework, master, implicitAck);

    status = (driver->run() == DRIVER_STOPPED) ? 0 : 1;

    driver->stop();

    delete driver;
    return status;
}
