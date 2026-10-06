#include <vector>
#include <cstddef>
#include <string>
#include <iostream>
#include <stdexcept>
#include <omp.h>
#include <random>
#include <algorithm>
#include <unordered_map>
#include "Local_Decition_Tree_Class.h"


#include <iomanip>
#include <algorithm>

void print_strategy(std::unordered_map<std::string, std::vector<float>>& stratagy) {
	auto labels = [](size_t n) -> std::vector<std::string> {
		if (n == 2) return { "check/fold", "bet/call" };
		return { "fold", "call", "raise" };
		};
	auto rank = [](char c) -> std::string {
		if (c == '0') return "J"; if (c == '1') return "Q"; if (c == '2') return "K";
		return std::string(1, c);
		};
	std::vector<std::string> keys;
	for (auto& e : stratagy) keys.push_back(e.first);
	std::sort(keys.begin(), keys.end());

	std::cout << "\n=============== AVERAGE STRATEGY ===============\n";
	std::cout << std::left << std::setw(22) << "infoset" << "|  action probabilities\n";
	std::cout << "-----------------------------------------------\n";
	for (auto& key : keys) {
		auto& vec = stratagy[key];
		std::string pretty = rank(key[0]) + key.substr(1);
		std::cout << std::left << std::setw(22) << pretty << "|  ";
		auto lab = labels(vec.size());
		for (size_t i = 0; i < vec.size(); i++) {
			std::cout << lab[i] << " " << std::fixed << std::setprecision(3) << vec[i];
			if (i + 1 < vec.size()) std::cout << "   ";
		}
		std::cout << "\n";
	}
	std::cout << "===============================================\n";
	std::cout << "total infosets: " << stratagy.size() << "\n";
}

std::vector<float> deck_maker(int n_players, int ranks_) {
	int j_loop = std::max( 2,(n_players + ranks_) / ranks_);
	std::vector<float> deck = {};
	for (int i = 0; i < ranks_; i++) {
		for (int j = 0; j < j_loop; j++) {
			deck.push_back(i);
		}
	}
	return deck;
}


class hand_ {
private:
	std::vector<float> deck_;
	std::vector<float> cards_;

	int n_players_;
	float p_;
	int ranks_;
public:
	hand_(std::vector<float> deck, std::vector<float> cards , int n_players, float p , int ranks )
		: deck_(deck), cards_(cards), n_players_(n_players), p_(p), ranks_(ranks) {}

	~hand_() {}

	void branch(std::vector<hand_>& Cards_Delt) {
		if (cards_.size() == n_players_ + 1) {
			Cards_Delt.push_back(hand_(deck_, cards_, n_players_, p_, ranks_));
		}
		else {
			for (float i = 0; i < ranks_; i++) {
				int n = std::count(deck_.begin(), deck_.end(), i);
				std::vector<float> deck = deck_;
				std::vector<float> cards = cards_;
				if (n > 0) {
					std::vector<float> new_deck = deck;
					auto it = std::find(new_deck.begin(), new_deck.end(), i);
					if (it != new_deck.end()) {
						new_deck.erase(it);
					}
					std::vector<float> new_cards = cards;
					new_cards.push_back(i);
					float new_p = p_ * (static_cast<float>(n) / deck_.size());
					hand_ child(new_deck, new_cards, n_players_, new_p, ranks_);
					child.branch(Cards_Delt);
				}
			}
		}
	}
	//accecability functions
	std::vector<float> cards() {
		return cards_;
	}
	float p() {
		return p_;
	}
};

	

int main() {
	int n_players = 3;
	int ranks = 3;
	std::vector<float> deck = deck_maker(n_players,ranks);
	std::vector<float> player_contributions(n_players, 1.0f);
	std::vector<std::string> player_status(n_players, "playing");
	std::vector<float> reach(n_players, 1.0f);
	std::vector<hand_> Cards_Delt;
	hand_ root(deck,{},n_players,1.0f,ranks);
	root.branch(Cards_Delt);

	std::unordered_map<std::string, std::vector<float>> regrets;
	std::unordered_map<std::string, std::vector<float>> strategy;

	int last_agressor = -1;
	bool round_raised = false;
	
	std::vector<std::string> history = {};
	std::vector<std::string> round_history = {};
	
	int player_turn = 0;
	float pot = n_players*1.0f;
	int round = 1;

	int n_generations = 50000;
	for (int j = 0; j < n_generations; j++) {
		for (int i = 0; i < Cards_Delt.size(); i++) {
			hand_ hand_i = Cards_Delt[i];
			std::vector<float> cards_ = hand_i.cards();
			float chance = hand_i.p();
			Decition_Node root = Decition_Node(last_agressor, round_raised, cards_, n_players, history, round_history, round, player_contributions, pot, player_status, player_turn);
			root.recursion(regrets, strategy, reach, chance);
		}
	}
	//final normalisation
	for (auto& entry : strategy) {
		const std::string& key = entry.first;   //id   
		std::vector<float>& vec = entry.second;     //stratagy
		float vec_total = 0;
		int I = vec.size();
		for (int i = 0; i < I; i++) {
			vec_total += vec[i];
		}
		if (vec_total > 0) {
			for (int i = 0; i < I; i++) {
				vec[i] /= vec_total;
			}
		}
		else {
			for (int i = 0; i < I; i++) vec[i] = 1.0f / I;
		}

	}
	print_strategy(strategy);

	std::vector<std::vector<float>> predetor_prey_exp_val(n_players, std::vector<float>(n_players, 0.0f));


	float chance = 1.0;
	for (int i = 0; i < n_players; i++) {
		for (int j = 0; j < ranks; j++) {
			std::vector<float> Cards(n_players + 1, 0.0f);
			Cards[i] = j;
			int predetor_card = j;
			std::vector<std::vector<float>> possible_cards = Decition_Node::possible_hands_calc(deck, {}, n_players, i, ranks, predetor_card,chance);
			std::vector<std::vector<float>> possible_cards_rip(possible_cards.begin(), possible_cards.end() - 1);
			std::vector<float> chance_vec = possible_cards.back();

			float chance_vec_sum = std::accumulate(chance_vec.begin(), chance_vec.end(), 0.0f);
			std::cout << "predator=" << i << " card=" << j
				<< "  chance_vec_sum=" << chance_vec_sum
				<< "  (expected ~" << 1.0f / ranks << ")\n";
			//exploitablity calc
			Decition_Node root(last_agressor, round_raised, Cards, n_players, history, round_history, round, player_contributions, pot, player_status, player_turn);
			std::vector<float> v = root.exploitiblity_calc(strategy, i, predetor_card, possible_cards_rip, chance_vec, ranks);
			for (int k = 0; k < n_players; k++) {
				predetor_prey_exp_val[i][k] += v[k];
			}
		}	
	}
	std::cout << "\n=============== EXPLOITABILITY ===============\n";
	std::cout << std::fixed << std::setprecision(6);
	std::cout << "Best-response value matrix (predetor_prey_exp_val[predator][player]):\n";
	for (int i = 0; i < n_players; i++) {
		std::cout << "  predator=" << i << ":  ";
		for (int k = 0; k < n_players; k++) {
			std::cout << "P" << k << "=" << predetor_prey_exp_val[i][k] << "   ";
		}
		std::cout << "\n";
	}
	std::cout << "-----------------------------------------------\n";
	std::cout << "Zero-sum check (each row should sum to ~0):\n";
	for (int i = 0; i < n_players; i++) {
		float row_sum = 0.0f;
		for (int k = 0; k < n_players; k++) row_sum += predetor_prey_exp_val[i][k];
		std::cout << "  predator=" << i << " row sum: " << row_sum << "\n";
	}
	std::cout << "-----------------------------------------------\n";
	float total_exploitability = 0.0f;
	for (int i = 0; i < n_players; i++) {
		std::cout << "Player " << i << " best-response value vs others' avg strategy: "
			<< predetor_prey_exp_val[i][i] << "\n";
		total_exploitability += predetor_prey_exp_val[i][i];
	}
	float exploitability = total_exploitability / n_players;
	std::cout << "-----------------------------------------------\n";
	std::cout << "Exploitability (chips/hand):        " << exploitability << "\n";
	std::cout << "Exploitability (milli-antes/hand):   " << exploitability * 1000.0f << "\n";
	std::cout << "===============================================\n";
	return 0;
}
