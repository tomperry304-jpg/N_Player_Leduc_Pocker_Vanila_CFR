#include <vector>
#include <cstddef>
#include <string>
#include <iostream>
#include <stdexcept>
#include <omp.h>  
#include <random>
#include <unordered_map>
#include <numeric>

class Decition_Node {
private:
    std::vector<float> cards_;
    std::vector<std::string> history_;
    std::vector<std::string> round_history_;
    int round_;
    std::vector<float> player_contributions_;
    int n_players_;
    float pot_;
    std::vector <std::string> valid_moves_;
    std::vector<std::string> player_status_;
    int player_turn_;
    int last_agressor_;
    bool round_raised_;
public:
    Decition_Node(int last_agressor, bool round_raised,std::vector<float> cards, int n_players, std::vector<std::string> history, std::vector<std::string> round_history, int round, std::vector<float> player_contributions, float pot, std::vector<std::string> player_status, int player_turn)
        : cards_(cards), history_(history), round_history_(round_history), round_(round), n_players_(n_players), player_contributions_(player_contributions), pot_(pot),player_status_(player_status),player_turn_(player_turn), last_agressor_(last_agressor), round_raised_(round_raised){

    }

    ~Decition_Node() {}

    //accecability funcitons
    int get_round() const { return round_; }
    int active_players() const { return std::count(player_status_.begin(), player_status_.end(), "playing"); }
    bool current_player_active() const { return player_status_[player_turn_] == "playing"; }

    bool checking_check(int n_seats) {
        bool check = true;
        for (int i = 2; i <= n_seats; i++) {
            if (round_history_[round_history_.size() - i] == "check") {
            }
            else {
                check = false;
                return check;
            }
        }
        return check;
    }

    int n_seats_() {
        return std::count(player_status_.begin(), player_status_.end(), "playing");
    }

    std::vector<std::string> legal_actions() {

        


        if (round_history_.empty()) {
            return { "check","bet" };
        }
        int n_seats = n_seats_();
        if (n_seats == 1) {
            return {};
        }

        const std::string& last = round_history_.back();
        if (last == "check") {
            if (round_history_.size() >= n_seats && checking_check(n_seats)) return {};
            return { "check","bet"};
        }

        if (last == "bet") {
            return { "fold","call","raise" };
        }
        if (last == "raise") {
            return { "fold","call" };
        }
        if (last == "fold" || last == "call") {
            if (player_turn_ == last_agressor_) {
                return{};
            }
            else {
                return { "fold","call" };
            }
        };
        return {};
    }

    float wining_card() {
        float public_card = cards_.back();
        float highest_card = -1e9;
        for (int i = 0; i < n_players_; i++) {
            if (cards_[i] == public_card and player_status_[i] == "playing") {
                return public_card;
            }
            else {
                if (cards_[i] > highest_card and player_status_[i] == "playing") {
                    highest_card = cards_[i];
                }
            }
        }
        return highest_card;
    }


    std::vector<float> terminal_returns() {
        std::vector<float> returns(player_contributions_.size(), 0.0f);
        int n_seats = std::count(player_status_.begin(), player_status_.end(), "playing");
        if (n_seats == 1) {
            auto it = std::find(player_status_.begin(), player_status_.end(), "playing");
            int winner_idx = std::distance(player_status_.begin(), it);
            for (int i = 0; i < n_players_; i++) {
                if (i == winner_idx) {
                    returns[i] = pot_ - player_contributions_[i];
                }
                else {
                    returns[i] = -1* player_contributions_[i];
                }
            }
        }
        else {
            float best_card = wining_card();

            std::vector<int> winner_i;
            for (int i = 0; i < n_players_; i++) {
                returns[i] = -1 * player_contributions_[i];
                if (cards_[i] == best_card && player_status_[i] == "playing") {
                    winner_i.push_back(i);
                }
            }
            for (int i = 0; i < winner_i.size(); i++) {
                returns[winner_i[i]] += pot_ / winner_i.size();
            }
        }
        return returns;
    }


    Decition_Node apply_action(std::string action) {
        //cards_, history_, round_history_, round_, n_players_, player_contributions_, pot_, player_status_, player_turn_, last_agressor_, round_raised_;
        float new_pot_ = pot_;
        std::vector<float> new_player_contributions_ = player_contributions_;
        std::vector<std::string> new_player_status_ = player_status_;
        std::vector<std::string> new_history_ = history_;
        std::vector<std::string> new_round_history_ = round_history_;
        int new_last_agressor_ = last_agressor_;
        bool new_round_raised_ = round_raised_;
        int new_player_turn_ = player_turn_;
        if (action == "raise" || action == "call" || action == "bet") {
            //walk back untill hit
            bool hit = false;
            int i = 1;
            while (hit == false) {
                if (new_player_status_[((player_turn_ - i) % n_players_ + n_players_) % n_players_] == "playing") {
                    hit = true;
                }
                else if (i > n_players_) throw std::runtime_error("no active player found");
                else {
                    i++;
                }
            }
            int I = ((player_turn_ - i) % n_players_ + n_players_) % n_players_;
            new_pot_ -= new_player_contributions_[player_turn_];
            if (action == "raise") {
                new_player_contributions_[player_turn_] = new_player_contributions_[I] + 2 * round_;
                new_last_agressor_ = player_turn_;
                new_round_raised_ = true;
            }
            else if (action == "bet") {
                new_player_contributions_[player_turn_] = new_player_contributions_[I] + 2 * round_;
                new_last_agressor_ = player_turn_;
            }
            else {
                new_player_contributions_[player_turn_] = new_player_contributions_[I];
            }
            new_pot_ += new_player_contributions_[player_turn_];
        }
        else if (action == "fold") {
            new_player_status_[player_turn_] = "folded";
        }
        new_history_.push_back(action);
        new_round_history_.push_back(action);
        //walk back untill hit
        bool hit = false;
        int i = 1;
        while (hit == false) {
            if (new_player_status_[(player_turn_ + i) % n_players_] == "playing") {
                hit = true;
            }
            else if (i > n_players_) throw std::runtime_error("no active player found");
            else {
                i++;
            }
        }
        new_player_turn_ = (player_turn_ + i) % n_players_;
        return Decition_Node(new_last_agressor_, new_round_raised_, cards_,n_players_, new_history_ , new_round_history_,round_,new_player_contributions_, new_pot_,new_player_status_, new_player_turn_);
    }


    Decition_Node new_round() {
        //cards_, history_, round_history_, round_, n_players_, player_contributions_, pot_, player_status_, player_turn_, last_agressor_, round_raised_;
        std::vector<std::string> new_round_history_ = {};
        bool new_round_raised_ = false;
        int new_player_turn_;
        int new_round_ = 2;
        
        bool hit = false;
        int i = 0;
        while (hit == false) {
            if (player_status_[i % n_players_] == "playing") {
                hit = true;
            }
            else if (i > n_players_) throw std::runtime_error("no active player found");
            else {
                i++;
            }
        }
        new_player_turn_ = i % n_players_;
        return Decition_Node(last_agressor_, new_round_raised_, cards_, n_players_, history_, new_round_history_, new_round_, player_contributions_, pot_, player_status_, new_player_turn_);
    }




    std::string get_key() {
        std::string key = std::to_string((int)cards_[player_turn_]);
        if (round_ >= 2) key += "|Public" + std::to_string((int)cards_[n_players_]);
        for (std::string& a : history_) key += "|" + a;
        return key;
    }

    std::string get_prey_key() {
        std::string key = std::to_string((int)cards_[player_turn_]);
        key += "|Public" + std::to_string((int)cards_[n_players_]);
        for (std::string& a : history_) key += "|" + a;
        return key;
    }


    
    std::vector<float> recursion(std::unordered_map<std::string, std::vector<float>>& regrets, std::unordered_map<std::string, std::vector<float>>& strategy, std::vector<float> reach , float chance) {
        std::vector<std::string> actions = legal_actions();
        if (actions.empty()) {
            if (n_seats_()!=1 && round_ == 1){
                Decition_Node child = new_round();
                return child.recursion(regrets, strategy, reach, chance);
            }
            else {
                return terminal_returns();
            }
        }
        else {
            std::vector<float> node_value(n_players_,0.0f);
            std::vector< std::vector<float>> exp_action_i;
            std::vector<float> opponent_reach(actions.size(),1.0f);
            std::string key = get_key();
            auto it = regrets.find(key);
            if (it == regrets.end()) {
                regrets[key] = std::vector<float>(actions.size(), 0.0f);
               strategy[key] = std::vector<float>(actions.size(), 0.0f);
            }
            std::vector<float> regret = regrets[key];
            float total_regret = 0;
            for (int i = 0; i < regret.size(); i++) {
                if (regret[i] <= 0) {
                    regret[i] = 0;
                }
                total_regret += regret[i];
            }
            for (int i = 0; i < actions.size(); i++) {
                Decition_Node child = apply_action(actions[i]);                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 

                float p = 0;
                if (total_regret == 0) {
                    p = 1.0f / regret.size();
                }
                else {
                    p = regret[i] / total_regret;
                }
                std::vector<float> new_reach = reach;
                
                for (int j = 0; j < reach.size(); j++) {
                    if (j != player_turn_) opponent_reach[i] *= reach[j];
                }
                new_reach[player_turn_] *= p;


                std::vector<float> exp = child.recursion(regrets, strategy, new_reach, chance);
                exp_action_i.push_back(exp);
                for (size_t i = 0; i < node_value.size(); i++) {
                    node_value[i] += p * exp[i];
                }
                
                strategy[key][i] += chance * reach[player_turn_] * p;
            }
            for (int i = 0; i < actions.size(); i++) {
                regrets[key][i] += chance * opponent_reach[i] * (exp_action_i[i][player_turn_] - node_value[player_turn_]);

            }


            return node_value;
        }
    }
    //####################################################################################################
    //####################################################################################################
    //####################################################################################################


    class predetor_hand_ {
    private:
        std::vector<float> deck_;
        std::vector<float> cards_;

        int n_players_;
        int predator_;
        float predator_card_;
        int unique_cards_;
        float chance_;
    public:
        predetor_hand_(std::vector<float> deck, std::vector<float> cards, int n_players, int predetor, int unique_cards, float predator_card, float chance)
            : deck_(deck), cards_(cards), n_players_(n_players), predator_(predetor), unique_cards_(unique_cards), predator_card_(predator_card),chance_(chance) {
        }
        ~predetor_hand_() {}

        std::vector<float> cards() {
            return cards_;
        }

        void branch(std::vector<std::vector<float>>& Possible_Cards, std::vector<float>& chance_vec) {
            if (cards_.size() == n_players_ + 1) {
                Possible_Cards.push_back(cards_);
                chance_vec.push_back(chance_);
            }
            else if (cards_.size() == predator_) {
                int n = std::count(deck_.begin(), deck_.end(), predator_card_);
                std::vector<float> deck = deck_;
                std::vector<float> cards = cards_;
                if (n > 0) {
                    std::vector<float> new_deck = deck;
                    auto it = std::find(new_deck.begin(), new_deck.end(), predator_card_);
                    if (it != new_deck.end()) {
                        new_deck.erase(it);
                    }
                    std::vector<float> new_cards = cards;
                    new_cards.push_back(predator_card_);
                    float new_chance = chance_ * (static_cast<float>(n) / deck_.size());
                    predetor_hand_ child(new_deck, new_cards, n_players_, predator_, unique_cards_, predator_card_, new_chance);
                    child.branch(Possible_Cards, chance_vec);
                }
            }
            else {
                for (float i = 0; i < unique_cards_; i++) {
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
                        float new_chance = chance_ * (static_cast<float>(n) / deck_.size());
                        predetor_hand_ child(new_deck, new_cards, n_players_, predator_, unique_cards_, predator_card_, new_chance);
                        child.branch(Possible_Cards, chance_vec);
                    }
                }
            }
        }
    };




   static std::vector<std::vector<float>> possible_hands_calc(std::vector<float> deck, std::vector<float> cards, int n_players, int predetor, int unique_cards, float predator_card, float chance) {
        std::vector< std::vector<float>> possible_cards{};
        std::vector<float> chance_vec{};
        predetor_hand_ root(deck, cards, n_players, predetor, unique_cards,predator_card,chance);
        root.branch(possible_cards, chance_vec);
        possible_cards.push_back(chance_vec);
        return possible_cards;
    }


    
    //###########################################################################################
    //###########################################################################################
    //###########################################################################################
    void build_prey_reach(std::unordered_map<std::string, std::vector<float>>& prey_reach_hash, std::unordered_map<std::string, std::vector<float>>& strategy, int predator, std::vector<std::vector<float>>&  possible_hands) {
        std::vector<std::string> actions = legal_actions();
        if (actions.empty()) {
            if (n_seats_() != 1 && round_ == 1) {
                Decition_Node child = new_round();
                child.build_prey_reach(prey_reach_hash, strategy, predator, possible_hands);
                return;
            }
            else {
                return;
            }
        }
        else {
            std::string key = get_key();
            auto it = strategy.find(key);
            if (it == strategy.end()) {
                throw std::runtime_error("no strategy with associated key: " + key);
            }


            std::vector<float> strategum = strategy[key];
            for (int i = 0; i < actions.size(); i++) {
                Decition_Node child = apply_action(actions[i]);
                if (player_turn_ != predator) {
                    std::string action_i_key = child.get_prey_key();
                    auto it = prey_reach_hash.find(action_i_key);
                    if (it == prey_reach_hash.end()) {
                        prey_reach_hash[action_i_key] = std::vector<float> (possible_hands.size(),1.0f);
                    }
                    auto match = std::find(possible_hands.begin(), possible_hands.end(), cards_);
                    if (match == possible_hands.end()) {
                        throw std::runtime_error("real hand not found");
                    }
                    int I = static_cast<int>(std::distance(possible_hands.begin(), match));
                    prey_reach_hash[action_i_key][I] *= strategum[i];
                }
                child.build_prey_reach(prey_reach_hash, strategy, predator, possible_hands);
            }
            return;
        }
    }


   
    




    //#########################################################################################################
    //#########################################################################################################
    //#########################################################################################################

    std::vector<float> exploitiblity_calc(std::unordered_map<std::string, std::vector<float>>& strategy, int predator, int predetor_card, std::vector<std::vector<float>> possible_hands, std::vector<float> weight_vec, int ranks) {
        std::vector<std::string> actions = legal_actions();
        if (actions.empty()) {
            if (n_seats_() != 1 && round_ == 1) {
                Decition_Node child = new_round();
                return child.exploitiblity_calc(strategy,predator,predetor_card,possible_hands, weight_vec, ranks);
            }
            else {
                std::vector<float> node_value(n_players_, 0.0f);
                for (int j = 0; j < possible_hands.size(); j++) {
                    Decition_Node child(last_agressor_, round_raised_, possible_hands[j], n_players_, history_, round_history_, round_, player_contributions_, pot_, player_status_, player_turn_);
                    std::string key_for_combo_j = child.get_key();
                    std::vector<float> v = child.terminal_returns();
                    for (int k = 0; k < n_players_; k++) {
                        node_value[k] += v[k]*weight_vec[j];
                    }
                }
                return node_value;
            }
        }
        else {
            std::vector<float> node_value(n_players_, 0.0f);
            if (round_ == 1 or player_turn_ != predator) {
                
                std::vector< std::vector<float>> exp_action_i;
                std::vector<float> oponent_reach;
                std::string key = get_key();

                float max_return = -1e9;
                
                for (int i = 0; i < actions.size(); i++) {

                    if (player_turn_ == predator) {
                        Decition_Node child = apply_action(actions[i]);
                        std::vector<float> v = child.exploitiblity_calc(strategy, predator, predetor_card, possible_hands, weight_vec, ranks);
                        if (v[predator] > max_return) {
                            node_value = v;
                            max_return = v[predator];
                        }
                    }
                    else {
                        Decition_Node root = apply_action(actions[i]);
                        std::vector<float> new_weight_vec = weight_vec;
                        for (int j = 0; j < possible_hands.size(); j++) {
                            Decition_Node child(last_agressor_, round_raised_, possible_hands[j], n_players_, history_, round_history_, round_, player_contributions_, pot_, player_status_, player_turn_);
                            std::string key_for_combo_j = child.get_key();
                            new_weight_vec[j] *= strategy.at(key_for_combo_j)[i];
                        }
                        std::vector<float> v = root.exploitiblity_calc(strategy, predator, predetor_card, possible_hands, new_weight_vec, ranks);
                        for (int k = 0; k < n_players_; k++) {
                            node_value[k] += v[k];
                        }
                    }
                }
            }
            else {
                for (int l = 0; l < ranks; l++) {
                    std::vector< std::vector<float>> filterd_hands;
                    std::vector<float> filtder_weights;
                    std::vector<float> filterd_node_value(n_players_, 0.0f);
                    for (int j = 0; j < possible_hands.size(); j++) {
                        if (possible_hands[j].back() == l) {
                            filterd_hands.push_back(possible_hands[j]);
                            filtder_weights.push_back(weight_vec[j]);
                        }
                    }
                    float max_return = -1e9;
                    for (int i = 0; i < actions.size(); i++) {
                        Decition_Node child = apply_action(actions[i]);
                        std::vector<float> v = child.exploitiblity_calc(strategy, predator, predetor_card, filterd_hands, filtder_weights, ranks);
                        if (v[predator] > max_return) {
                            filterd_node_value = v;
                            max_return = v[predator];
                        }
                    }
                    for (int k = 0; k < n_players_; k++) {
                        node_value[k] += filterd_node_value[k];
                    }
                }
            }
            return node_value;
        }
    }
};