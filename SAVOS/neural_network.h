#ifndef NEURAL_NETWORK_H
#define NEURAL_NETWORK_H

#include <vector>
using namespace std;

class NeuralNetwork {
public:
    NeuralNetwork(vector<int>& layer_sizes, double lr);

    // Основні методи
    vector<double> forward(vector<double>& input);
    void train(vector<double>& input, vector<double>& target);
    void train_batch(vector<vector<double>>& inputs, vector<vector<double>>& targets, int epochs);
    double calculate_loss(vector<vector<double>>& inputs, vector<vector<double>>& targets);
    void print_weights();

private:
    double learning_rate;
    int input_size, output_size;
    vector<int> hidden_sizes;

    // Кожен елемент weights[i] – це матриця ваг для з'єднання (шар i) -> (шар i+1)
    vector< vector< vector<double> > > weights;
    // biases[i] – вектор зсувів для шару i+1 (тобто для шару, який приймає сигнал з попереднього)
    vector< vector<double> > biases;
    // layers[i] – значення (активації) нейронів шару i, де шар 0 – вхідний
    vector< vector<double> > layers;

    // Допоміжні функції для навчання
    vector< vector<double> > compute_deltas(vector<double>& target);
    void update_weights_and_biases(int layer, vector<double>& prev_layer_values, vector<double>& deltas);
};

#endif
