#include "seperating_matrix.hpp"

#include <cassert>
#include <functional>
#include <queue>

using namespace std;

seperating_matrix create_all_pair_seperating_sequences(const splitting_tree & root){
	const auto N = root.states.size();
	seperating_matrix all_pair_seperating_sequences(N, seperating_row(N));

	queue<reference_wrapper<const splitting_tree>> work;
	work.push(root);

	// total complexity is O(n^2), as we're visiting each pair only once :)
	while(!work.empty()){
		const splitting_tree & node = work.front();
		work.pop();

		auto it = begin(node.children);
		auto ed = end(node.children);

		while(it != ed){
			auto jt = next(it);
			while(jt != ed){
				for(auto && s : it->states){
					for(auto && t : jt->states){
						assert(all_pair_seperating_sequences[t.base()][s.base()].empty());
						assert(all_pair_seperating_sequences[s.base()][t.base()].empty());
						all_pair_seperating_sequences[t.base()][s.base()] = node.seperator;
						all_pair_seperating_sequences[s.base()][t.base()] = node.seperator;
					}
				}
				jt++;
			}
			it++;
		}

		for(auto && c : node.children){
			work.push(c);
		}
	}

	for(size_t i = 0; i < N; ++i){
		for(size_t j = 0; j < N; ++j){
			if(i == j) continue;
			assert(!all_pair_seperating_sequences[i][j].empty());
		}
	}

	return all_pair_seperating_sequences;
}
