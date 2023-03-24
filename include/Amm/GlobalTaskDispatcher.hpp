//
// Created on 21/3/23.
//

#ifndef GAMM_GLOBALTASKDISPATCHER_HPP
#define GAMM_GLOBALTASKDISPATCHER_HPP

#include <cstddef>
#include <memory>
#include <mutex>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <Eigen/Dense>

#include "Amm/Bamm.hpp"
#include "BS_thread_pool.hpp"
#include "Svd/SequentialJTS.hpp"
#include "Utils/ZeroedColumns.hpp"
#include "Single.hpp"


namespace GAMM {
    struct Task {
        enum Type { Setup, SvdStep, Finish };
        Type type;
        int index; // Only used for SvdStep tasks

        Task(Type type, int index = -1) : type(type), index(index) {}
    };

    class DAG {
    public:
        void addEdge(Task u, Task v);
        std::vector<int> topologicalSort() const;
        void dfs(Task v, std::vector<bool>& visited, std::deque<Task>& result);
        std::deque<Task> topologicalSort();

    private:
        std::vector<std::vector<Task>> adjList = std::vector<std::vector<Task>>(Task::Finish + 1);

    };

    class GlobalTaskDispatcher : public Bamm {
        struct LockedMatrices {
            std::mutex mtx;
            MatrixPtr bx, by;
        };

        BS::thread_pool_ptr pool;
        std::vector<LockedMatrices> matrices;
        std::barrier<> barrier;

        size_t getT() const { return pool->get_thread_count() + 1; }

    public:
        GlobalTaskDispatcher(size_t l, scalar_t beta, BS::thread_pool_ptr pool)
                : Bamm(l, beta, std::make_unique<SequentialJTS>()), pool{pool},
                  barrier{pool->get_thread_count() + 1} {}

        void reduce() override;


        void dispatch_task(GAMM::Single bamm, DAG dag);
    };


}
#endif //GAMM_GLOBALTASKDISPATCHER_HPP
