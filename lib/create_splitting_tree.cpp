#include "create_splitting_tree.hpp"

#include <functional>
#include <numeric>
#include <queue>
#include <utility>

using namespace std;

template <typename T>
std::vector<T> concat(std::vector<T> const & l, std::vector<T> const & r){
	std::vector<T> ret(l.size() + r.size());
	auto it = copy(begin(l), end(l), begin(ret));
	copy(begin(r), end(r), it);
	return ret;
}

result create_splitting_tree(const Mealy& g){
	const auto N = g.graph.size();
	const auto P = g.input_indices.size();
	const auto Q = g.output_indices.size();

	result r(N);
	auto & root = r.root;
	auto & succession = r.successor_cache;

	/* We'll use a queue to keep track of leaves we have to investigate;
	 * In some cases we cannot split, and have to wait for other parts of the
	 * tree. We keep track of how many times we did no work. If this is too
	 * much, there is no complete splitting tree.
	 */
	queue<reference_wrapper<splijtboom>> work;
	size_t days_without_progress = 0;

	// Some lambda functions capturing some state, makes the code a bit easier :)
	const auto add_push_new_block = [&work](auto new_blocks, auto & boom) {
		boom.children.assign(new_blocks.size(), splijtboom(0, boom.depth + 1));

		auto i = 0;
		for(auto && b : new_blocks){
			boom.children[i++].states.assign(begin(b), end(b));
		}

		for(auto && c : boom.children){
			work.push(c);
		}

		assert(boom.states.size() == accumulate(begin(boom.children), end(boom.children), 0, [](auto l, auto r) { return l + r.states.size(); }));
	};
	const auto is_valid = [N, &g](auto blocks, auto symbol){
		for(auto && block : blocks) {
			const auto new_blocks = partition_(begin(block), end(block), [symbol, &g](state state){
				return apply(g, state, symbol).to.base();
			}, N);
			for(auto && new_block : new_blocks){
				if(new_block.size() != 1) return false;
			}
		}
		return true;
	};
	const auto update_succession = [N, &succession](state s, state t, size_t depth){
		if(succession.size() < depth+1) succession.resize(depth+1, vector<state>(N, -1));
		succession[depth][s.base()] = t;
	};

	// We'll start with the root, obviously
	work.push(root);
	while(!work.empty()){
		splijtboom & boom = work.front();
		work.pop();
		const auto depth = boom.depth;

		if(boom.states.size() == 1) continue;

		// First try to split on output
		for(input symbol = 0; symbol < P; ++symbol){
			const auto new_blocks = partition_(begin(boom.states), end(boom.states), [symbol, depth, &g, &update_succession](state state){
				const auto ret = apply(g, state, symbol);
				update_succession(state, ret.to, depth);
				return ret.output.base();
			}, Q);

			// no split -> continue with other input symbols
			if(new_blocks.size() == 1) continue;

			// not a valid split -> continue
			if(!is_valid(new_blocks, symbol)) continue;

			// a succesful split, update partition and add the children
			boom.seperator = {symbol};
			add_push_new_block(new_blocks, boom);

			goto has_split;
		}

		// Then try to split on state
		for(input symbol = 0; symbol < P; ++symbol){
			vector<bool> successor_states(N, false);
			for(auto && state : boom.states){
				successor_states[apply(g, state, symbol).to.base()] = true;
			}

			const auto & oboom = lca(root, [&successor_states](state state) -> bool{
				return successor_states[state.base()];
			});

			// a leaf, hence not a split -> try other symbols
			if(oboom.children.empty()) continue;

			// possibly a succesful split, construct the children
			const auto word = concat({symbol}, oboom.seperator);
			const auto new_blocks = partition_(begin(boom.states), end(boom.states), [word, depth, &g, &update_succession](state state){
				const auto ret = apply(g, state, begin(word), end(word));
				update_succession(state, ret.to, depth);
				return ret.output.base();
			}, Q);

			// not a valid split -> continue
			if(!is_valid(new_blocks, symbol)) continue;

			assert(new_blocks.size() > 1);

			// update partition and add the children
			boom.seperator = word;
			add_push_new_block(new_blocks, boom);

			goto has_split;
		}

		// We tried all we could, but did not succeed => declare incompleteness.
		if(days_without_progress++ >= work.size()) {
			r.is_complete = false;
			return r;
		}
		work.push(boom);
		continue;

		has_split:
		days_without_progress = 0;
	}

	return r;
}
