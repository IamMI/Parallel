#include "tasksys.h"
#include <atomic>
#include <thread>
#include <mutex>
#include <iostream>
#include <typeinfo>
#include <vector>
#include <algorithm>

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
    for (int i = 0; i < num_total_tasks; i++) {
        runnable->runTask(i, num_total_tasks);
    }

    return 0;
}

void TaskSystemSerial::sync() {
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

TaskSystemParallelSpawn::TaskSystemParallelSpawn(int num_threads): ITaskSystem(num_threads), m_max_threads{num_threads} {
    // NOTE: CS149 students are not expected to implement TaskSystemParallelSpawn in Part B.
}

TaskSystemParallelSpawn::~TaskSystemParallelSpawn() {}

void TaskSystemParallelSpawn::run(IRunnable* runnable, int num_total_tasks) {
    for (int i = 0; i < num_total_tasks; i++) {
        runnable->runTask(i, num_total_tasks);
    }
}

TaskID TaskSystemParallelSpawn::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                                 const std::vector<TaskID>& deps) {
    // NOTE: CS149 students are not expected to implement TaskSystemParallelSpawn in Part B.
    for (int i = 0; i < num_total_tasks; i++) {
        runnable->runTask(i, num_total_tasks);
    }

    return 0;
}

void TaskSystemParallelSpawn::sync() {
    // NOTE: CS149 students are not expected to implement TaskSystemParallelSpawn in Part B.
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

TaskSystemParallelThreadPoolSpinning::TaskSystemParallelThreadPoolSpinning(int num_threads): ITaskSystem(num_threads), m_max_threads(num_threads) {
    //
    // CS149 student implementations may decide to perform setup
    // operations (such as thread pool construction) here.
    // Implementations are free to add new class member variables
    // (requiring changes to tasksys.h).
    //
}

TaskSystemParallelThreadPoolSpinning::~TaskSystemParallelThreadPoolSpinning() {
    
}


void TaskSystemParallelThreadPoolSpinning::run(IRunnable* runnable, int num_total_tasks) {
    //
    // CS149 students will modify the implementation of this
    // method in Part A.  The implementation provided below runs all
    // tasks sequentially on the calling thread.
    //
    for (int i = 0; i < num_total_tasks; i++) {
        runnable->runTask(i, num_total_tasks);
    }
}

TaskID TaskSystemParallelThreadPoolSpinning::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                                              const std::vector<TaskID>& deps) {
    // NOTE: CS149 students are not expected to implement TaskSystemParallelThreadPoolSpinning in Part B.
    for (int i = 0; i < num_total_tasks; i++) {
        runnable->runTask(i, num_total_tasks);
    }

    return 0;
}

void TaskSystemParallelThreadPoolSpinning::sync() {
    // NOTE: CS149 students are not expected to implement TaskSystemParallelThreadPoolSpinning in Part B.
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

TaskSystemParallelThreadPoolSleeping::TaskSystemParallelThreadPoolSleeping(int num_threads): 
    ITaskSystem(num_threads), m_max_threads{num_threads}
{
    // Initialize threads pool
    for (int i = 0; i < this->m_max_threads; i++) {
        this->worker_pool.push_back(std::thread([this]() {
            worker();
        }));
    }
}

TaskSystemParallelThreadPoolSleeping::~TaskSystemParallelThreadPoolSleeping() {
    this->stop = true;
    cv_work.notify_all();
    
    for (auto& worker : this->worker_pool) {
        if (worker.joinable()) {
            worker.join(); 
        }
    }
    
    for (int index=0; index<this->m_num_contexts; index++) {
        delete this->task_contexts[index];
    }
}

void TaskSystemParallelThreadPoolSleeping::worker() {
    while (true) {
        // At run time, worker would wait for a context to run, otherwise it sleeps.
        // When a context is available, worker would fetch a context from runnable_contexts
        // and run the context. After finishing the context, worker would notify the 
        // context_thread to remove this context.

        std::unique_lock<std::mutex> lock(m_work);
        auto wait_func = [this] { return this->stop || \
                                         static_cast<int>(this->runnable_contexts.size()) > 0; };        
        this->cv_work.wait(lock, wait_func);

        if (stop && static_cast<int>(this->runnable_contexts.size()) == 0) {
            break;
        }

        // Fetch a runnable context from this->runnable_contexts
        TaskContext* task_context = nullptr;
        for (int index=0; index<static_cast<int>(this->runnable_contexts.size()); index++){
            if (this->runnable_contexts[index]->m_left_tasks > 0){
                task_context = this->runnable_contexts[index];
                break;
            }
        }
        if (task_context == nullptr) {
            continue;
        }

        int taskId = task_context->m_num_tasks - task_context->m_left_tasks;
        task_context->m_left_tasks --;
        IRunnable* runner = task_context->m_runnable;
        int m_num_tasks = task_context->m_num_tasks;

        lock.unlock();
        
        runner->runTask(taskId, m_num_tasks);
        {
            std::unique_lock<std::mutex> lock(task_context->m_mtx);
            task_context->m_finish_tasks ++;
            if (task_context->m_finish_tasks == task_context->m_num_tasks) {
                {
                    // Remove runnable context from this->runnable_contexts
                    std::unique_lock<std::mutex> lock(this->m_work);
                    this->runnable_contexts.erase(std::remove(this->runnable_contexts.begin(), this->runnable_contexts.end(), task_context), this->runnable_contexts.end());
                }
                task_context->is_finished = true;
                task_context->m_cv.notify_all();        // Notify context_thread to remove this context
            }
        }
    }
}

void TaskSystemParallelThreadPoolSleeping::run(IRunnable* runnable, int num_total_tasks) {
    // run function is similar with runAsyncWithDeps, but without dependencies.
    // In the final, run has wait for the task_context to finish.

    int cur_task_id = this->next_task_id.fetch_add(1);
    TaskContext* task_context = new TaskContext(cur_task_id, {}, runnable, num_total_tasks);
    this->task_contexts[cur_task_id] = task_context;
    this->m_num_contexts ++;

    // Add runnable context to this->runnable_contexts
    {
        std::unique_lock<std::mutex> lock(this->m_work);
        this->runnable_contexts.push_back(task_context);
        this->cv_work.notify_all();
    }
    // Wait for task_context to finish
    {
        std::unique_lock<std::mutex> lock(task_context->m_mtx);
        task_context->m_cv.wait(lock, [task_context]() { return task_context->is_finished; });
    }  
    // // Remove runnable context from this->runnable_contexts
    // {
    //     std::unique_lock<std::mutex> lock(this->m_work);
    //     this->runnable_contexts.erase(std::remove(this->runnable_contexts.begin(), this->runnable_contexts.end(), task_context), this->runnable_contexts.end());
    // } 
}


TaskID TaskSystemParallelThreadPoolSleeping::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
    const std::vector<TaskID>& deps) {
    // At run time, runAsyncWithDeps would generate a new task id and a task_context to record
    // metadata. Then, a context thread is pushed into this->context_pool, which responds for:
    //      1. Wait util the deps finished
    //      2. Push task context into this->runnable_contexts 
    //      3. Notify worker threads to fetch and carry on

    int cur_task_id = this->next_task_id.fetch_add(1);
    TaskContext* task_context = new TaskContext(cur_task_id, deps, runnable, num_total_tasks);
    this->task_contexts[cur_task_id] = task_context;    // Record
    this->m_num_contexts ++;                            // Number of total contexts

    task_context->m_thread = std::thread([this, task_context]() {
            // Waiting for deps
            TaskContext* task_deps = nullptr;
            if (task_context->m_deps.size() > 0){
                for (auto& dep : task_context->m_deps){
                    {
                        task_deps = this->task_contexts[dep];
                        std::unique_lock<std::mutex> lock(task_deps->m_mtx);
                        task_deps->m_cv.wait(lock, [task_deps]() { return task_deps->is_finished; });
                    }
                }
            }
            // Add runnable context to this->runnable_contexts
            {
                std::unique_lock<std::mutex> lock(this->m_work);
                this->runnable_contexts.push_back(task_context);
                this->cv_work.notify_all();
            }
    });
    
    return cur_task_id;
}



void TaskSystemParallelThreadPoolSleeping::sync() { 
    for (int index=0; index< static_cast<int>(this->m_num_contexts); index++) {
        TaskContext* context = this->task_contexts[index];

        std::unique_lock<std::mutex> lock(context->m_mtx);

        context->m_cv.wait(lock, [context]() { return context->is_finished; });

        if (context->m_thread.joinable()) {
            context->m_thread.join();
        }
    }

    return;
}
