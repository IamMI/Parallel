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
            m_num_tasks{num_total_tasks}, m_left_tasks{num_total_tasks}, m_finish_tasks{0}, \
            is_finished{false}, \
            m_context_ids{task_id}, m_deps{deps}, m_runnable{runnable}
        {}
        ~TaskContext() {}
        const int m_num_tasks{0};               // Number of total tasks
        int m_left_tasks{0};                    // Number of left tasks
        int m_finish_tasks{0};                  // Number of finished tasks
        bool is_finished{false};                // Indicate whether context is done    

        TaskID m_context_ids;                   // Context Id
        std::vector<TaskID> m_deps;             // Context's dependencies    

        std::condition_variable m_cv;           // Conditional variable to notify other contexts
        std::mutex m_mtx;                       // Mutex to protect data

        std::thread m_thread;                   // Thread to carry out task
        IRunnable* m_runnable;                  // Runnable program
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
        std::atomic<int> next_task_id{0};           // Symbol of next task Id
        TaskContext* task_contexts[10000];           // TaskContexts
        std::vector<TaskContext*> runnable_contexts;// Runnable contexts
        std::atomic<int> m_num_contexts{0};         // Number of total contexts
        // std::atomic<int> m_left_task_total{0};      // Number of total left tasks

        std::vector<std::thread> worker_pool;       // Threads pool
        bool stop{false};                           // Indicate whether to stop
        
        std::mutex m_work;                          // Mutex to protect data
        std::condition_variable cv_work, cv_finish; // To notify task_context->m_thread and run
};

#endif
