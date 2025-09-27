#ifndef NEUTRAL_GAMEENGINE_JOB_SYSTEM_H
#define NEUTRAL_GAMEENGINE_JOB_SYSTEM_H

#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <atomic>
#include <functional>
#include <unordered_set>
#include <unordered_map>

namespace FoundryEngine {

using JobID = uint32_t;
using JobPriority = int;

struct Job {
    JobID id;
    std::function<void()> task;
    JobPriority priority;
    std::vector<JobID> dependencies;
    std::atomic<int> dependentCount;

    Job(JobID id_, std::function<void()> task_, JobPriority pri = 0)
        : id(id_), task(task_), priority(pri), dependentCount(0) {}
};

class JobScheduler {
public:
    JobScheduler(size_t numThreads = std::thread::hardware_concurrency());
    ~JobScheduler();

    // Schedule a job
    JobID scheduleJob(std::function<void()> task, JobPriority priority = 0);

    // Schedule job with dependencies
    JobID scheduleJobWithDeps(std::function<void()> task, const std::vector<JobID>& deps, JobPriority priority = 0);

    // Wait for job completion
    void waitForJob(JobID jobId);
    void waitForAll();

    // Add dependency
    void addDependency(JobID dependent, JobID dependency);

private:
    std::vector<std::thread> workers_;
    std::priority_queue<std::pair<JobPriority, JobID>, std::vector<std::pair<JobPriority, JobID>>, std::greater<>> jobQueue_;
    std::unordered_map<JobID, Job> jobs_;
    std::mutex queueMutex_;
    std::condition_variable cv_;
    std::atomic<bool> stop_;
    JobID nextJobID_;
    std::mutex mapMutex_;

    // Completed jobs tracking
    std::unordered_set<JobID> completedJobs_;
    std::mutex completedMutex_;
    std::condition_variable completedCV_;

    void workerThread();
    bool isJobReady(JobID jobId);
};

} // namespace FoundryEngine

#endif // NEUTRAL_GAMEENGINE_JOB_SYSTEM_H
