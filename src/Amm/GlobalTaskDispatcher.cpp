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

//TODO improve efficiency
void GlobalTaskDispatcher::dispatch_task(std::vector<Single> bamms) {
    std::deque<Task> sortedTasks;
    std::vector<std::deque<Task>> sortedList;
    int idx = 0;
    for (auto& bamm : bamms){
        sortedList[idx] = this->dags[idx].topologicalSort();
        ++idx;
    }
    std::vector<std::future<bool>> futures;
    while (true) {
        idx = 0;
        for (auto &bamm: bamms) {
            sortedTasks = sortedList[idx];
            //TODO algorithm that improves the utilization of threadpool
            Task task = sortedTasks.front();
            sortedTasks.pop_front();

            if (&futures[idx]) {
                futures[idx].wait();
                if (futures[idx].get() == false) {
                    // TODO handle outcome
                    /*while (!sortedTasks.empty() && sortedTasks.front().type == Task::SvdStep) {
                        sortedTasks.pop_front();
                    }*/
                }
            }

            switch (task.type) {
                case Task::Setup:
                    futures[idx] = pool->submit([this, &bamm] {
                        return bamm.Bamm::reductionStepSetup();
                    });
                    break;
                case Task::SvdStep:
                    futures[idx] = pool->submit([this, &bamm, task] {
                        return bamm.Bamm::reductionStepSvdStep(task.index);
                    });
                    break;
                case Task::Finish:
                    futures[idx] = pool->submit([this, &bamm] {
                        return bamm.Bamm::reductionStepFinish();
                    });
                    break;
            }
            ++idx;
        }
    }
}

void GlobalTaskDispatcher::reduce() {
    auto t = getT();

    // initially split the matrix t times
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
    for (int i = 0; i < t; ++i) {
        Single bamm = getBamm(i, t-i);
        auto maxSweeps = bamm.getMaxSweeps();
        Task setup(Task::Setup);
        Task finish(Task::Finish);
        dags[i].addEdge(setup, finish);

        // svd steps
        for (int j = 0; i < maxSweeps; ++i) {
            Task svdStep(Task::SvdStep, j);
            dags[i].addEdge(setup, svdStep);
            dags[i].addEdge(svdStep, finish);
        }

        bamms.push_back(std::move(bamm));
    }
    dispatch_task(std::move(bamms));

    /*while (!bamm.Bamm::reductionStepSetup()) {
        for (int i = 0; i < maxSweeps; ++i) {
            if (!bamm.Bamm::reductionStepSvdStep(i)) break;
        }
        bamm.Bamm::reductionStepFinish();
    }*/

    matrices.clear();
}

Single GlobalTaskDispatcher::getBamm(int id1, int id2) {
    auto &ownMatrices = matrices[id1];
    std::optional<MatrixRef> xcols, ycols;
    xcols = matrices[id2].bx->leftCols(l);
    ycols = matrices[id2].by->leftCols(l);
    Single bamm(l, beta);
    bamm.setMatrices(std::move(xcols.value()), std::move(ycols.value()),
                     ownMatrices.bx, ownMatrices.by);
    return bamm;
}



