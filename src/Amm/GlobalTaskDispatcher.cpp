//
// Created on 21/3/23.
//

#include "Amm/Bamm.hpp"
#include "AMM/GlobalTaskDispatcher.hpp"
#include "Amm/Single.hpp"

using namespace GAMM;




void DAG::addEdge(Task u, Task v) {
    adjList[u.type].push_back(v);
}

void DAG::dfs(Task v, std::vector<bool>& visited, std::deque<Task>& result) {
    visited[v.type] = true;

    for (const Task& neighbor : adjList[v.type]) {
        if (!visited[neighbor.type]) {
            dfs(neighbor, visited, result);
        }
    }

    result.push_front(v);
}

//TODO add multilayer sort
std::deque<Task> DAG::topologicalSort() {
    std::deque<Task> result;
    std::vector<bool> visited(Task::Finish + 1, false);

    for (int i = 0; i <= Task::Finish; ++i) {
        if (!visited[i]) {
            dfs(Task(static_cast<Task::Type>(i)), visited, result);
        }
    }

    return result;
}

void GlobalTaskDispatcher::dispatch_task(Single bamm) {
    std::deque<Task> sortedTasks = this->dags[0].topologicalSort();
    // BS::multi_future<bool> tasks(sortedTasks.size());
    std::future<bool> future;
    //TODO algorithm that improves the utilization of threadpool
    while (!sortedTasks.empty()) {
        Task task = sortedTasks.front();
        sortedTasks.pop_front();
        switch (task.type) {
            case Task::Setup:
                future = pool->submit([this, &bamm] {
                    return bamm.Bamm::reductionStepSetup();
                });
                break;
            case Task::SvdStep:
                future = pool->submit([this, &bamm, task] {
                    return bamm.Bamm::reductionStepSvdStep(task.index);
                });
                break;
            case Task::Finish:
                future = pool->submit([this, &bamm] {
                    return bamm.Bamm::reductionStepFinish();
                });
                break;
        }
        future.wait();
        // delete the chain of the rest of svd steps
        if (future.get() == false) {
            while (!sortedTasks.empty() && sortedTasks.front().type == Task::SvdStep) {
                sortedTasks.pop_front();
            }
        }
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

    //TODO tree structure matrix splitting and reduction
    auto &ownMatrices = matrices[0];
    std::optional<MatrixRef> xcols, ycols;
    xcols = matrices[1].bx->leftCols(l);
    ycols = matrices[1].by->leftCols(l);
    Single bamm(l, beta);
    bamm.setMatrices(std::move(xcols.value()), std::move(ycols.value()),
                     ownMatrices.bx, ownMatrices.by);

    auto maxSweeps = bamm.getMaxSweeps();

    /*while (!bamm.Bamm::reductionStepSetup()) {
        for (int i = 0; i < maxSweeps; ++i) {
            if (!bamm.Bamm::reductionStepSvdStep(i)) break;
        }
        bamm.Bamm::reductionStepFinish();
    }*/

    Task setup(Task::Setup);
    Task finish(Task::Finish);
    dags[0].addEdge(setup, finish);

    // svd steps
    for (int i = 0; i < maxSweeps; ++i) {
        Task svdStep(Task::SvdStep, i);
        dags[0].addEdge(setup, svdStep);
        dags[0].addEdge(svdStep, finish);
    }

    dispatch_task(std::move(bamm));

    matrices.clear();
}




