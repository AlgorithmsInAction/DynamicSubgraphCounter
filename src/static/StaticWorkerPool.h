#ifndef STATIC_WORKER_POOL_H
#define STATIC_WORKER_POOL_H

#include "static/StaticAlgorithm.h"

#include <algorithm>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <exception>
#include <map>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

class StaticWorkerPool {
  public:
    StaticWorkerPool(unsigned int worker_count, const StaticAlgorithm &algorithm)
        : algorithm_(algorithm), max_queued_(worker_count * 2) {
        workers_.reserve(worker_count);
        for (unsigned int i = 0; i < worker_count; ++i)
            workers_.emplace_back([this] { work(); });
    }

    ~StaticWorkerPool() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            stopping_ = true;
        }
        jobs_available_.notify_all();
        queue_space_.notify_all();
        for (auto &worker : workers_)
            worker.join();
    }

    StaticWorkerPool(const StaticWorkerPool &) = delete;
    StaticWorkerPool &operator=(const StaticWorkerPool &) = delete;

    void submit(StaticUpdateBlock block) {
        std::unique_lock<std::mutex> lock(mutex_);
        queue_space_.wait(lock, [&] {
            return jobs_.size() < max_queued_ || worker_error_;
        });
        rethrow_worker_error();
        jobs_.push_back(std::move(block));
        jobs_available_.notify_one();
    }

    StaticAlgorithmResult take(int step) {
        std::unique_lock<std::mutex> lock(mutex_);
        result_available_.wait(lock, [&] {
            return results_.find(step) != results_.end() || worker_error_;
        });
        rethrow_worker_error();
        auto found = results_.find(step);
        auto result = std::move(found->second);
        results_.erase(found);
        return result;
    }

  private:
    using Edge = std::pair<int, int>;

    static std::uint64_t edge_key(int tail, int head) {
        if (head < tail)
            std::swap(tail, head);
        return (std::uint64_t(static_cast<std::uint32_t>(tail)) << 32) |
               static_cast<std::uint32_t>(head);
    }

    void compute_block(StaticUpdateBlock block) {
        const auto initial_edge_count = block.checkpoint.num_edges();
        std::vector<Edge> edges;
        edges.reserve(initial_edge_count + block.steps.size());
        std::unordered_map<std::uint64_t, std::size_t> positions;
        positions.reserve(initial_edge_count + block.steps.size());

        for (unsigned int i = 0; i < initial_edge_count; ++i) {
            int tail = block.checkpoint.edges[i];
            int head = block.checkpoint.edges[i + initial_edge_count];
            if (head < tail)
                std::swap(tail, head);
            positions.emplace(edge_key(tail, head), edges.size());
            edges.emplace_back(tail, head);
        }

        int step_number = block.first_step;
        std::vector<std::pair<int, StaticAlgorithmResult>> block_results;
        block_results.reserve(block.steps.size());
        for (const auto &step : block.steps) {
            for (const auto &update : step.updates) {
                int tail = update.tail;
                int head = update.head;
                if (head < tail)
                    std::swap(tail, head);
                const auto key = edge_key(tail, head);
                auto found = positions.find(key);

                if (update.insertion) {
                    if (found == positions.end()) {
                        positions.emplace(key, edges.size());
                        edges.emplace_back(tail, head);
                    }
                } else if (found != positions.end()) {
                    const auto removed_position = found->second;
                    const auto last_edge = edges.back();
                    edges[removed_position] = last_edge;
                    positions[edge_key(last_edge.first, last_edge.second)] =
                        removed_position;
                    edges.pop_back();
                    positions.erase(found);
                }
            }

            StaticGraphSnapshot snapshot;
            snapshot.num_vertices = step.num_vertices;
            snapshot.edges.resize(edges.size() * 2);
            for (std::size_t i = 0; i < edges.size(); ++i) {
                snapshot.edges[i] = edges[i].first;
                snapshot.edges[i + edges.size()] = edges[i].second;
            }

            block_results.emplace_back(
                step_number, algorithm_.compute_snapshot(snapshot));
            ++step_number;
        }

        {
            std::lock_guard<std::mutex> lock(mutex_);
            for (auto &result : block_results)
                results_.emplace(result.first, std::move(result.second));
        }
        result_available_.notify_all();
    }

    void rethrow_worker_error() const {
        if (worker_error_)
            std::rethrow_exception(worker_error_);
    }

    void work() {
        while (true) {
            StaticUpdateBlock block;
            {
                std::unique_lock<std::mutex> lock(mutex_);
                jobs_available_.wait(
                    lock, [&] { return stopping_ || !jobs_.empty(); });
                if (stopping_ && jobs_.empty())
                    return;
                block = std::move(jobs_.front());
                jobs_.pop_front();
                queue_space_.notify_one();
            }

            try {
                compute_block(std::move(block));
            } catch (...) {
                {
                    std::lock_guard<std::mutex> lock(mutex_);
                    if (!worker_error_)
                        worker_error_ = std::current_exception();
                    stopping_ = true;
                    jobs_.clear();
                }
                jobs_available_.notify_all();
                result_available_.notify_all();
                queue_space_.notify_all();
                return;
            }
        }
    }

    const StaticAlgorithm &algorithm_;
    const std::size_t max_queued_;
    std::vector<std::thread> workers_;
    std::deque<StaticUpdateBlock> jobs_;
    std::map<int, StaticAlgorithmResult> results_;
    mutable std::mutex mutex_;
    std::condition_variable jobs_available_;
    std::condition_variable queue_space_;
    std::condition_variable result_available_;
    std::exception_ptr worker_error_;
    bool stopping_{false};
};

#endif
