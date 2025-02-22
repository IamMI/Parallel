#include "tasksys.h"
#include <atomic>
#include <thread>
#include <mutex>
#include <iostream>
#include <typeinfo>

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

TaskSystemParallelThreadPoolSleeping::TaskSystemParallelThreadPoolSleeping(int num_threads): ITaskSystem(num_threads), m_max_threads{num_threads} {
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
            worker.join(); // 等待所有线程完成
        }
    }

    // 释放所有任务上下文
    for (auto& pair : this->task_contexts) {
        delete pair;
    }
}

void TaskSystemParallelThreadPoolSleeping::worker() {
    while (true) {
        std::unique_lock<std::mutex> lock(m_work);
        auto wait_func = [this] { return this->stop || this->m_left_num > 0; };
        cv_work.wait(lock, wait_func);

        if (stop && m_left_num == 0) {
            break;
        }

        int task_id = m_total_num - m_left_num;
        m_left_num--;
        lock.unlock();

        runner->runTask(task_id, m_total_num);
        {
            std::lock_guard<std::mutex> finish_lock(m_finish);
            m_finished_num++;
            if (m_finished_num == m_total_num) {
                cv_finish.notify_one();
            }
        }
    }
}

void TaskSystemParallelThreadPoolSleeping::run(IRunnable* runnable, int num_total_tasks) {
    runner = runnable;
    m_finished_num = 0;
    m_total_num = num_total_tasks;

    {
        std::lock_guard<std::mutex> lock(m_work);
        m_left_num = num_total_tasks;
    }

    cv_work.notify_all();

    std::unique_lock<std::mutex> lock(m_finish);
    auto wait_func = [this](){return this->m_finished_num == this->m_total_num;};
    cv_finish.wait(lock, wait_func);
}


TaskID TaskSystemParallelThreadPoolSleeping::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
    const std::vector<TaskID>& deps) {

    int cur_task_id = this->next_task_id.fetch_add(1);
    TaskContext* task_context = new TaskContext(cur_task_id, deps, runnable, num_total_tasks);
    this->task_contexts.push_back(task_context);
    

    task_context->m_thread = std::thread([this, task_context]() {
            TaskContext* task_deps = nullptr;
            if (task_context->m_deps.size() > 0){
                for (auto& dep : task_context->m_deps){
                    if (dep < static_cast<int>(this->task_contexts.size())) {
                        task_deps = this->task_contexts[dep];
                        std::unique_lock<std::mutex> lock(task_deps->m_mtx);
                        task_deps->m_cv.wait(lock, [task_deps]() { return task_deps->is_finished; });
                    }
                }
            }

            {
                std::unique_lock<std::mutex> lock(this->m_tasks);
                this->run(task_context->m_runnable, task_context->m_num_tasks);
                task_context->is_finished = true;
                task_context->m_cv.notify_all();
            }
    });
    return cur_task_id;
}



void TaskSystemParallelThreadPoolSleeping::sync() { 
    for (auto& context : this->task_contexts) {
        std::unique_lock<std::mutex> lock(context->m_mtx);
        
        context->m_cv.wait(lock, [context]() { return context->is_finished; });
        
        if(context->m_thread.joinable()) {
            context->m_thread.join();
        }
    }

    return;
}
