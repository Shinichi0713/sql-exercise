#include <iostream>
#include <vector>
#include <random>
#include <algorithm>
#include <iomanip>

// 状態と行動の定義
const int GRID_SIZE = 4;
const int NUM_STATES = GRID_SIZE * GRID_SIZE;
const int NUM_ACTIONS = 4; // 0: Up, 1: Down, 2: Left, 3: Right

// 行動移動量のオフセット (Up, Down, Left, Right)
const int dr[] = {-1, 1, 0, 0};
const int dc[] = {0, 0, -1, 1};

// 環境クラス
class GridWorld {
public:
    int state;
    const int goal_state = NUM_STATES - 1; // 右下 (3, 3) がゴール

    GridWorld() { reset(); }

    void reset() {
        state = 0; // 左上 (0, 0) からスタート
    }

    // 次の状態と報酬を返す
    std::pair<int, double> step(int action) {
        int r = state / GRID_SIZE;
        int c = state % GRID_SIZE;

        int nr = r + dr[action];
        int nc = c + dc[action];

        // 盤面内に収まる場合のみ移動
        if (nr >= 0 && nr < GRID_SIZE && nc >= 0 && nc < GRID_SIZE) {
            state = nr * GRID_SIZE + nc;
        }

        // ゴールに達したら報酬100、それ以外はステップ毎に-1のペナルティ
        if (state == goal_state) {
            return {state, 100.0};
        }
        return {state, -1.0};
    }

    bool is_done() const {
        return state == goal_state;
    }
};

// Q学習エージェントクラス
class QLearningAgent {
private:
    double alpha; // 学習率
    double gamma; // 割引率
    double epsilon; // ε-greedy法の探索率
    std::mt19937 rng;

public:
    std::vector<std::vector<double>> q_table;

    QLearningAgent(double alpha = 0.1, double gamma = 0.99, double epsilon = 0.1)
        : alpha(alpha), gamma(gamma), epsilon(epsilon),
          q_table(NUM_STATES, std::vector<double>(NUM_ACTIONS, 0.0)),
          rng(std::random_device{}()) {}

    // ε-greedyによる行動選択
    int select_action(int state) {
        std::uniform_real_distribution<double> dist(0.0, 1.0);
        if (dist(rng) < epsilon) {
            std::uniform_int_distribution<int> action_dist(0, NUM_ACTIONS - 1);
            return action_dist(rng); // ランダム探索
        } else {
            // Greedy選択（最大Q値の行動を抽出）
            auto max_it = std::max_element(q_table[state].begin(), q_table[state].end());
            return std::distance(q_table[state].begin(), max_it);
        }
    }

    // Q値の更新: Q(s, a) <- Q(s, a) + alpha * [r + gamma * max_a' Q(s', a') - Q(s, a)]
    void update(int state, int action, double reward, int next_state, bool done) {
        double max_next_q = 0.0;
        if (!done) {
            max_next_q = *std::max_element(q_table[next_state].begin(), q_table[next_state].end());
        }
        double target = reward + gamma * max_next_q;
        q_table[state][action] += alpha * (target - q_table[state][action]);
    }
};

int main() {
    GridWorld env;
    QLearningAgent agent(0.1, 0.99, 0.2); // alpha, gamma, epsilon

    const int EPISODES = 500;

    for (int ep = 0; ep < EPISODES; ++ep) {
        env.reset();
        int total_reward = 0;

        while (!env.is_done()) {
            int state = env.state;
            int action = agent.select_action(state);
            auto [next_state, reward] = env.step(action);

            agent.update(state, action, reward, next_state, env.is_done());
            total_reward += reward;
        }

        if ((ep + 1) % 100 == 0) {
            std::cout << "Episode " << std::setw(3) << ep + 1 
                      << " | Total Reward: " << total_reward << std::endl;
        }
    }

    // 学習後の最適行動出力 (0: Up, 1: Down, 2: Left, 3: Right)
    char action_chars[] = {'^', 'v', '<', '>'};
    std::cout << "\nLearned Policy Grid (0,0 to 3,3 Goal):\n";
    for (int r = 0; r < GRID_SIZE; ++r) {
        for (int c = 0; c < GRID_SIZE; ++c) {
            int s = r * GRID_SIZE + c;
            if (s == env.goal_state) {
                std::cout << " G ";
            } else {
                auto max_it = std::max_element(agent.q_table[s].begin(), agent.q_table[s].end());
                int best_action = std::distance(agent.q_table[s].begin(), max_it);
                std::cout << " " << action_chars[best_action] << " ";
            }
        }
        std::cout << "\n";
    }

    return 0;
}