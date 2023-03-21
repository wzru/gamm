//
// Created on 21/3/23.
//

#ifndef GAMM_GLOBALTASKDISPATCHER_HPP
#define GAMM_GLOBALTASKDISPATCHER_HPP

#include <cstddef>
#include <memory>
#include <mutex>
#include <optional>

#include <Eigen/Dense>

#include "Amm/Bamm.hpp"
#include "BS_thread_pool.hpp"
#include "Svd/SequentialJTS.hpp"
#include "Utils/ZeroedColumns.hpp"

namespace GAMM {
    class TaskDAG{
    public:

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

        void workerTask(size_t workerId);

    public:
        GlobalTaskDispatcher(size_t l, scalar_t beta, BS::thread_pool_ptr pool)
                : Bamm(l, beta, std::make_unique<SequentialJTS>()), pool{pool},
                  barrier{pool->get_thread_count() + 1} {}

        void reduce() override;
        std::deque<int> topological_sort(std::shared_ptr<TaskDAG> dag);
    };
}



#endif //GAMM_GLOBALTASKDISPATCHER_HPP
