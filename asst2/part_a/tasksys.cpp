#include "tasksys.h"
#include <thread>
#include <cmath>
#include <mutex>
#include <condition_variable>
#include <iostream>

IRunnable::~IRunnable() {}

ITaskSystem::ITaskSystem(int num_threads) {}
ITaskSystem::~ITaskSystem() {}

/*
 * ================================================================
 * Serial task system implementation
 * ================================================================
 */

const char* TaskSystemSerial::name() {
    return "Serial";
}

TaskSystemSerial::TaskSystemSerial(int num_threads): ITaskSystem(num_threads) {
}

TaskSystemSerial::~TaskSystemSerial() {}

void TaskSystemSerial::run(IRunnable* runnable, int num_total_tasks) {
    for (int i = 0; i < num_total_tasks; i++) {
        runnable->runTask(i, num_total_tasks);
    }
}

TaskID TaskSystemSerial::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                          const std::vector<TaskID>& deps) {
    // You do not need to implement this method.
    return 0;
}

void TaskSystemSerial::sync() {
    // You do not need to implement this method.
    return;
}

/*
 * ================================================================
 * Parallel Task System Implementation
 * ================================================================
 */

const char* TaskSystemParallelSpawn::name() {
    return "Parallel + Always Spawn";
}

TaskSystemParallelSpawn::TaskSystemParallelSpawn(int num_threads): ITaskSystem(num_threads) {
    //
    // TODO: CS149 student implementations may decide to perform setup
    // operations (such as thread pool construction) here.
    // Implementations are free to add new class member variables
    // (requiring changes to tasksys.h).
    //
    m_num_threads = num_threads;
}

TaskSystemParallelSpawn::~TaskSystemParallelSpawn() {}

void TaskSystemParallelSpawn::run(IRunnable* runnable, int num_total_tasks) {
    //
    // TODO: CS149 students will modify the implementation of this
    // method in Part A.  The implementation provided below runs all
    // tasks sequentially on the calling thread.
    //
    
    std::thread threads[m_num_threads];
    int count=0;
    // std::cout << "num_total_tasks are " << num_total_tasks << std::endl;
    while(count<num_total_tasks){
        for(int i=0;i<m_num_threads;i++){
            if(count<num_total_tasks){
                threads[i] = std::thread(&IRunnable::runTask,runnable,count,num_total_tasks);
                count++;
            }
        }
        
        for(int i=0;i<m_num_threads;i++){
            if(threads[i].joinable()){
                threads[i].join();
            }
        }
        
    }
    
    
}

TaskID TaskSystemParallelSpawn::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                                 const std::vector<TaskID>& deps) {
    // You do not need to implement this method.
    return 0;
}

void TaskSystemParallelSpawn::sync() {
    // You do not need to implement this method.
    return;
}

/*
 * ================================================================
 * Parallel Thread Pool Spinning Task System Implementation
 * ================================================================
 */

const char* TaskSystemParallelThreadPoolSpinning::name() {
    return "Parallel + Thread Pool + Spin";
}

TaskSystemParallelThreadPoolSpinning::TaskSystemParallelThreadPoolSpinning(int num_threads): ITaskSystem(num_threads) {
    //
    // TODO: CS149 student implementations may decide to perform setup
    // operations (such as thread pool construction) here.
    // Implementations are free to add new class member variables
    // (requiring changes to tasksys.h).
    //
    m_num_threads = num_threads;
    m_count = 0;
    m_mtx = new std::mutex();
}

TaskSystemParallelThreadPoolSpinning::~TaskSystemParallelThreadPoolSpinning() {
    delete m_mtx;
}

void TaskSystemParallelThreadPoolSpinning::threadFunc(int thread_id, IRunnable* runnable, int num_total_tasks) {
    // Threads run functions synchronously
    int task_id;
    while (true) {
        {
            std::lock_guard<std::mutex> lock(*m_mtx);
            if(m_count >= num_total_tasks){
                break;
            }
            task_id = m_count++;
        }
        runnable->runTask(task_id, num_total_tasks);
    }
}

void TaskSystemParallelThreadPoolSpinning::run(IRunnable* runnable, int num_total_tasks) {
    //
    // TODO: CS149 students will modify the implementation of this
    // method in Part A.  The implementation provided below runs all
    // tasks sequentially on the calling thread.
    //
    m_count = 0;
    std::thread threads[m_num_threads];
    for (int index=0; index<m_num_threads; index++) {
        threads[index] = std::thread(&TaskSystemParallelThreadPoolSpinning::threadFunc, this, index, runnable, num_total_tasks);
    }
    for (int index=0; index<m_num_threads; index++) {
        threads[index].join();
    }

}

TaskID TaskSystemParallelThreadPoolSpinning::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                                              const std::vector<TaskID>& deps) {
    // You do not need to implement this method.
    return 0;
}

void TaskSystemParallelThreadPoolSpinning::sync() {
    // You do not need to implement this method.
    return;
}

/*
 * ================================================================
 * Parallel Thread Pool Sleeping Task System Implementation
 * ================================================================
 */

const char* TaskSystemParallelThreadPoolSleeping::name() {
    return "Parallel + Thread Pool + Sleep";
}

TaskSystemParallelThreadPoolSleeping::TaskSystemParallelThreadPoolSleeping(int num_threads): ITaskSystem(num_threads) {
    //
    // TODO: CS149 student implementations may decide to perform setup
    // operations (such as thread pool construction) here.
    // Implementations are free to add new class member variables
    // (requiring changes to tasksys.h).
    //
    m_num_threads = num_threads;
    m_count = 0;
    m_completed_tasks = 0;
    m_mtx = new std::mutex();
}

TaskSystemParallelThreadPoolSleeping::~TaskSystemParallelThreadPoolSleeping() {
    //
    // TODO: CS149 student implementations may decide to perform cleanup
    // operations (such as thread pool shutdown construction) here.
    // Implementations are free to add new class member variables
    // (requiring changes to tasksys.h).
    //
    
    delete m_mtx;
}

void TaskSystemParallelThreadPoolSleeping::threadFunc(int thread_id, IRunnable* runnable, int num_total_tasks) {
    // Threads run functions synchronously
    int task_id;
    while (true) {
        {
            std::lock_guard<std::mutex> lock(*m_mtx);
            if(m_count >= num_total_tasks){
                break;
            }
            task_id = m_count++;
        }
        runnable->runTask(task_id, num_total_tasks);

        {
            std::lock_guard<std::mutex> lock(*m_mtx);
            m_completed_tasks++;
            if(m_completed_tasks == num_total_tasks){
                m_cv.notify_all();
            }
        }
    }
}


void TaskSystemParallelThreadPoolSleeping::run(IRunnable* runnable, int num_total_tasks) {
    //
    // TODO: CS149 students will modify the implementation of this
    // method in Parts A and B.  The implementation provided below runs all
    // tasks sequentially on the calling thread.
    //
    
    m_count = 0;
    m_completed_tasks = 0;
    std::thread threads[m_num_threads];
    for (int index=0; index<m_num_threads; index++) {
        threads[index] = std::thread(&TaskSystemParallelThreadPoolSleeping::threadFunc, this, index, runnable, num_total_tasks);
    }
    
    {
        std::unique_lock<std::mutex> lk(*m_mtx);
        m_cv.wait(lk, [this, num_total_tasks] { return m_completed_tasks==num_total_tasks; });
    }
    for (int index=0; index<m_num_threads; index++) {
        threads[index].join();
    }
}

TaskID TaskSystemParallelThreadPoolSleeping::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                                    const std::vector<TaskID>& deps) {
    //
    // TODO: CS149 students will implement this method in Part B.
    //

    return 0;
}

void TaskSystemParallelThreadPoolSleeping::sync() {

    //
    // TODO: CS149 students will modify the implementation of this method in Part B.
    //

    return;
}




