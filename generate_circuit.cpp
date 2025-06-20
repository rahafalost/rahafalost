#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <set>
#include <numeric>
#include "mapping.h"
#include <utility>
#include <cmath>
#include <string>
#include <algorithm>

void flush_stage(const std::vector<std::pair<int, int>>& stage) {
    for (const auto& sg : stage) {
        if (sg.second == -1)
            std::cout << "(" << sg.first << ") ";
        else
            std::cout << "(" << sg.first << " " << sg.second << ") ";
    }
    std::cout << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc < 4) {
        std::cerr << "Usage: ./generate_circuit <pattern> <nqubits> <ngates>" << std::endl;
        return 1;
    }

    std::string pattern = argv[1];
    int nqubits = std::stoi(argv[2]);
    int ngates = std::stoi(argv[3]);

    if (nqubits <= 1 || ngates <= 0) {
        std::cerr << "Error: nqubits must be > 1 and ngates > 0." << std::endl;
        return 1;
    }

    srand(static_cast<unsigned>(time(0)));
    std::vector<std::pair<int, int>> gates;


    if (pattern == "hotspot") {
    std::vector<std::vector<int>> core_qubits = {
        {0, 16, 32, 48, 64, 80, 96},  {1, 17, 33, 49, 65, 81, 97},
        {2, 18, 34, 50, 66, 82, 98},  {3, 19, 35, 51, 67, 83, 99},
        {4, 20, 36, 52, 68, 84},      {5, 21, 37, 53, 69, 85},
        {6, 22, 38, 54, 70, 86},      {7, 23, 39, 55, 71, 87},
        {8, 24, 40, 56, 72, 88},      {9, 25, 41, 57, 73, 89},
        {10, 26, 42, 58, 74, 90},     {11, 27, 43, 59, 75, 91},
        {12, 28, 44, 60, 76, 92},     {13, 29, 45, 61, 77, 93},
        {14, 30, 46, 62, 78, 94},     {15, 31, 47, 63, 79, 95}
    };

    // Remove invalid qubits (greater than or equal to nqubits)
    for (auto& core : core_qubits) {
        core.erase(std::remove_if(core.begin(), core.end(),
            [nqubits](int q) { return q >= nqubits; }),
            core.end());
    }

    std::vector<int> core5_qubits = core_qubits[5];
    std::vector<int> neighbor_cores = {1, 4, 6, 9};

    int num_one_qubit = static_cast<int>(0.25 * ngates);
    int num_two_qubit = ngates - num_one_qubit;

    while (num_one_qubit > 0 || num_two_qubit > 0) {
        bool insert_two_qubit = (num_two_qubit > 0) &&
            (rand() % 4 < 3 || num_one_qubit == 0); 

        if (insert_two_qubit) {
            int type = rand() % 3;

            if (type == 0 && core5_qubits.size() >= 2) {
                // 2Q local gate inside core 5
                int q1 = core5_qubits[rand() % core5_qubits.size()];
                int q2 = core5_qubits[rand() % core5_qubits.size()];
                while (q1 == q2)
                    q2 = core5_qubits[rand() % core5_qubits.size()];
                gates.emplace_back(q1, q2);
                num_two_qubit--;
            } else if (type == 1 && !core5_qubits.empty()) {
                // 2Q inter-core gate between core 5 and neighbor
                int src = core5_qubits[rand() % core5_qubits.size()];
                int dst_core = neighbor_cores[rand() % neighbor_cores.size()];
                const auto& dst_qubits = core_qubits[dst_core];
                if (!dst_qubits.empty()) {
                    int dst = dst_qubits[rand() % dst_qubits.size()];
                    gates.emplace_back(src, dst);
                    num_two_qubit--;
                }
            } else {
                // 2Q random gate
                int src = rand() % nqubits;
                int dst = rand() % nqubits;
                while (src == dst)
                    dst = rand() % nqubits;
                gates.emplace_back(src, dst);
                num_two_qubit--;
            }
        } else {
            // 1Q gate on random qubit
            int q = rand() % nqubits;
            gates.emplace_back(q, -1);
            num_one_qubit--;
        }
    }
}



    // Schedule gates into disjoint stages
    std::set<int> busy;
    std::vector<std::pair<int, int>> stage;

    for (const auto& g : gates) {
        int src = g.first, dst = g.second;
        if (busy.count(src) == 0 && busy.count(dst) == 0) {
            stage.push_back(g);
            busy.insert(src);
            busy.insert(dst);
        } else {
            flush_stage(stage);
            stage.clear();
            busy.clear();
            stage.push_back(g);
            busy.insert(src);
            busy.insert(dst);
        }
    }

    if (!stage.empty())
        flush_stage(stage);

    return 0;
}
