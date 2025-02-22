#ifndef _TASKSYS_H
#define _TASKSYS_H

#include "itasksys.h"
#include <condition_variable>
#include <thread>
#include <atomic>

/*
 * TaskSystemSerial: This class is the student's implementation of a
 * serial task execution engine.  See definition of ITaskSystem in
 * itasksys.h for documentation of the ITaskSystem interface.
 */
class TaskSystemSerial: public ITaskSystem {
    public:
        TaskSystemSerial(int num_threads);
        ~TaskSystemSerial();
        const char* name();
        void run(IRunnable* runnable, int num_total_tasks);
        TaskID runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                const std::vector<TaskID>& deps);
        void sync();
};

/*
 * TaskSystemParallelSpawn: This class is the student's implementation of a
 * parallel task execution engine that spawns threads in every run()
 * call.  See definition of ITaskSystem in itasksys.h for documentation
 * of the ITaskSystem interface.
 */
class TaskSystemParallelSpawn: public ITaskSystem {
    public:
        TaskSystemParallelSpawn(int num_threads);
        ~TaskSystemParallelSpawn();
        const char* name();
        void run(IRunnable* runnable, int num_total_tasks);
        TaskID runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                const std::vector<TaskID>& deps);
        void sync();

        // Additional variables
        int m_max_threads{0};
};

/*
 * TaskSystemParallelThreadPoolSpinning: This class is the student's
 * implementation of a parallel task execution engine that uses a
 * thread pool. See definition of ITaskSystem in itasksys.h for
 * documentation of the ITaskSystem interface.
 */
class TaskSystemParallelThreadPoolSpinning: public ITaskSystem {
    public:
        TaskSystemParallelThreadPoolSpinning(int num_threads);
        ~TaskSystemParallelThreadPoolSpinning();
        const char* name();
        void run(IRunnable* runnable, int num_total_tasks);
        TaskID runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                const std::vector<TaskID>& deps);
        void sync();
        
        // Additional variables
        int m_max_threads{0};
};

/*
 * TaskSystemParallelThreadPoolSleeping: This class is the student's
 * optimized implementation of a parallel task execution engine that uses
 * a thread pool. See definition of ITaskSystem in
 * itasksys.h for documentation of the ITaskSystem interface.
 */


// We define a new data structure TaskContext, which contains:
// 1.Task id 2.Mutex 3.Conditional variable 4.Task's deps
// 5.is_finished to indicate whether task is done
// 6.Thread which responds to carry out task itself
// 7.Runnable task
// 8.Number of tasks

class TaskContext {
    public:
        TaskContext(int task_id, std::vector<TaskID> deps, IRunnable* runnable, int num_total_tasks): 
            m_task_ids{task_id}, m_deps{deps}, m_runnable{runnable}, m_num_tasks{num_total_tasks}
        {
            is_finished = false;
        }
        ~TaskContext() {}
        int m_num_tasks{0};
        int m_task_ids;
        std::condition_variable m_cv;
        std::mutex m_mtx;
        bool is_finished{false};
        std::thread m_thread;
        std::vector<TaskID> m_deps;
        IRunnable* m_runnable;
};


class TaskSystemParallelThreadPoolSleeping: public ITaskSystem {
    public:
        TaskSystemParallelThreadPoolSleeping(int num_threads);
        ~TaskSystemParallelThreadPoolSleeping();
        const char* name();
        void run(IRunnable* runnable, int num_total_tasks);
        TaskID runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                const std::vector<TaskID>& deps);
        void sync();
        void worker();
        int m_max_threads{0};
    private:
        std::atomic<int> next_task_id{0};           // 用于标志下一个任务的id
        std::vector<std::thread> worker_pool;       // 用于存放线程池
        bool stop{false};                           // 用于标志线程池是否停止
        
        std::mutex m_tasks;                         // 用于唤醒runAsyncWithDeps的线程
        std::mutex m_work;                          // 用于唤醒worker进行工作
        std::mutex m_finish;                        // 用于唤醒run函数，表示所有子任务完成
        IRunnable* runner;                          // 用于标志当前任务
        int m_left_num{0};                          // 用于标志剩余任务数
        int m_total_num{0};                         // 用于标志总任务数
        int m_finished_num{0};                      // 用于标志已完成任务数

        std::condition_variable cv_work, cv_finish; // 用于唤醒线程
        std::vector<TaskContext*> task_contexts;    // 用于存放任务上下文
};

#endif
