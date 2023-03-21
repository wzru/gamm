//
// Created on 21/3/23.
//

#include "Amm/Bamm.hpp"
#include "AMM/GlobalTaskDispatcher.hpp"

using namespace GAMM;

std::deque<int> topological_sort(std::shared_ptr<TaskDAG> dag){
    std::deque<int> sorted;
    // TODO implement algorithm
    return sorted;
}

void execute_task(std::shared_ptr<TaskDAG> dag){
    std::deque<int> sorted = topological_sort(dag);
    BS::multi_future<void> tasks(sorted.size());
    // TODO implement algorithm
    for (int i : sorted){
        // TODO map to task and assign to threadpool

    }
}

void GlobalTaskDispatcher::reduce() {
    auto t = getT();

    std::vector<LockedMatrices> temp{t};
    matrices.swap(temp);

    matrices[0].bx = bx.value();
    matrices[0].by = by.value();
    for (auto i = matrices.begin() + 1; i != matrices.end(); ++i) {
        i->bx = std::make_shared<Matrix>(x.value().rows(), l);
        i->by = std::make_shared<Matrix>(y.value().rows(), l);
    }

    auto d = x.value().cols();
    if (t * l > (size_t)d) {
        INTELLI_WARNING("Abort. The matrix has been split below the sketch size: "
                                << t << " * " << l << " > " << d);
        return;
    }

    //generate task DAG and pass to dispatcher
    auto dag = std::make_shared<TaskDAG>();
    //TODO assign task to dag

    execute_task(dag);

    matrices.clear();
}



