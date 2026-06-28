#include "neural_network.h"
#include "framework.h"
#include <iostream>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include <numeric>
#include <string>
#include <fstream>

using namespace std;

//-----------------------
//Активаційні функції та їх похідні
//-----------------------
double sigmoid(double x) {
    return 1.0 / (1.0 + exp(-x));
}

double sigmoid_derivative(double x) {
    //Припускаємо, що x вже є активацією (sigmoid(z))
    return x * (1.0 - x);
}

//-----------------------
//Лінійне перетворення
//-----------------------
//Обчислює зважену суму: z = W*x + b
double weighted_sum(const vector<double>& inputs, const vector<double>& weights, double bias) {
    double sum = bias;
    for (size_t i = 0; i < inputs.size(); i++) {
        sum += inputs[i] * weights[i];
    }
    return sum;
}

//-----------------------
//Функції для обчислення втрат
//-----------------------
double mse_loss(const vector<double>& target, const vector<double>& output) {
    double sum = 0.0;
    for (size_t i = 0; i < target.size(); i++) {
        double error = target[i] - output[i];
        sum += error * error;
    }
    return sum / target.size();
}

//-----------------------
//Функції для оновлення ваг (градієнтний спуск)
//-----------------------
void update_weights(vector<double>& weights, double gradient, const vector<double>& inputs, double learning_rate) {
    for (size_t i = 0; i < weights.size(); i++) {
        weights[i] += learning_rate * gradient * inputs[i];
        weights[i] = max(-1.0, min(1.0, weights[i]));
    }
}

void update_bias(double& bias, double gradient, double learning_rate) {
    bias += learning_rate * gradient;
}

//-----------------------
//Конструктор
//-----------------------
NeuralNetwork::NeuralNetwork(vector<int>& layer_sizes, double lr) {
    this->learning_rate = lr;
    input_size = layer_sizes[0];
    output_size = layer_sizes.back();
    hidden_sizes = vector<int>(layer_sizes.begin() + 1, layer_sizes.end() - 1);

    srand(time(0));

    //Ініціалізація ваг і зсувів для з'єднань між шарами
    //Кількість з'єднань = hidden_sizes.size() + 1
    for (int i = 0; i < hidden_sizes.size() + 1; ++i) {
        int prev_layer_size = (i == 0) ? input_size : hidden_sizes[i - 1];
        int current_layer_size = (i == hidden_sizes.size()) ? output_size : hidden_sizes[i];

        //Ініціалізація матриці ваг
        vector<vector<double>> layer_weights(current_layer_size, vector<double>(prev_layer_size));
        for (auto& row : layer_weights) {
            for (auto& w : row) {
                w = (double)rand() / RAND_MAX * 2 - 1;
            }
        }
        weights.push_back(layer_weights);

        //Ініціалізація зсувів
        vector<double> layer_biases(current_layer_size);
        for (auto& b : layer_biases) {
            b = (double)rand() / RAND_MAX * 2 - 1;
        }
        biases.push_back(layer_biases);

        //Ініціалізація значень шару
        vector<double> layer_values(current_layer_size, 0.0);
        layers.push_back(layer_values);
    }

    //Вхідний шар (layers[0]) буде заповнюватись під час forward
}

//-----------------------
//Forward-прохід: обчислюємо активації по шарах
//-----------------------
vector<double> NeuralNetwork::forward(vector<double>& input) {
    //Вхідний шар: копіюємо input у layers[0]
    for (size_t i = 0; i < input_size; ++i) {
        layers[0][i] = input[i];
    }
    //Для кожного наступного шару (від 1 до останнього)
    for (size_t layer = 1; layer < layers.size(); ++layer) {
        for (size_t i = 0; i < layers[layer].size(); ++i) {
            //Обчислюємо зважену суму для нейрона i цього шару
            double z = weighted_sum(layers[layer - 1], weights[layer][i], biases[layer][i]);
            //Застосовуємо функцію активації (тут sigmoid, але можна адаптувати)
            layers[layer][i] = sigmoid(z);
        }
    }
    return layers.back();
}

//-----------------------
//Обчислення дельт для всіх шарів (зворотній прохід)
//-----------------------
vector<vector<double>> NeuralNetwork::compute_deltas(vector<double>& target) {
    vector<vector<double>> deltas(layers.size());

    //Дельти для вихідного шару
    deltas.back().resize(output_size);
    for (size_t i = 0; i < output_size; ++i) {
        double error = target[i] - layers.back()[i];
        deltas.back()[i] = error * sigmoid_derivative(layers.back()[i]);
    }

    //Дельти для прихованих шарів (від останнього прихованого до першого)
    for (int layer = hidden_sizes.size() - 1; layer >= 0; --layer) {
        deltas[layer].resize(hidden_sizes[layer]);
        for (size_t i = 0; i < hidden_sizes[layer]; ++i) {
            double delta_sum = 0.0;
            //Наступний шар: якщо поточний останній прихований, то наступний – вихідний; інакше наступний прихований
            size_t next_size = (layer == hidden_sizes.size() - 1) ? output_size : hidden_sizes[layer + 1];
            for (size_t j = 0; j < next_size; ++j) {
                delta_sum += deltas[layer + 1][j] * weights[layer + 1][j][i];
            }
            deltas[layer][i] = delta_sum * sigmoid_derivative(layers[layer][i]);
        }
    }
    return deltas;
}

//-----------------------
//Оновлення ваг і зсувів для одного шару за допомогою градієнтного спуску
//-----------------------
void NeuralNetwork::update_weights_and_biases(int layer, vector<double>& prev_layer_values, vector<double>& delta) {
    //Оновлення ваг для з'єднання (шар layer-1 -> шар layer)
    for (size_t i = 0; i < delta.size(); ++i) {
        update_weights(weights[layer][i], delta[i], prev_layer_values, learning_rate);
        update_bias(biases[layer][i], delta[i], learning_rate);
    }
}

//-----------------------
//Функція навчання для одного прикладу
//-----------------------
void NeuralNetwork::train(vector<double>& input, vector<double>& target) {
    forward(input);
    vector<vector<double>> deltas = compute_deltas(target);
    //Оновлюємо ваги для кожного шару (weights[0] відповідає зв'язку між входом та першим прихованим шаром, і так далі)
    //Для шару 0 використовуємо input як значення попереднього шару
    for (size_t layer = 0; layer < weights.size(); ++layer) {
        vector<double>& prev_values = (layer == 0) ? input : layers[layer - 1];
        update_weights_and_biases(layer, prev_values, deltas[layer]);
    }
}

//-----------------------
//Функція пакетного навчання
//-----------------------
void NeuralNetwork::train_batch(vector<vector<double>>& inputs, vector<vector<double>>& targets, int epochs) {
    isTraining = true;
    for (int epoch = 0; epoch < epochs; epoch++) {
        for (size_t i = 0; i < inputs.size(); i++) {
            train(inputs[i], targets[i]);
        }
    }
    isTraining = false;
    auto W = get_weights();
    std::ofstream ofs("weights.txt");
    for (size_t l = 0; l < W.size(); ++l) {
        ofs << "Layer " << l << ":\n";
        for (size_t n = 0; n < W[l].size(); ++n) {
            for (double w : W[l][n]) ofs << w << " ";
            ofs << "\n";
        }
    }
    MessageBoxW(nullptr, L"Training complete!", L"Training", MB_OK);
}

//-----------------------
//Обчислення втрат
//-----------------------
double NeuralNetwork::calculate_loss(vector<vector<double>>& inputs, vector<vector<double>>& targets) {
    double loss = 0.0;
    for (size_t i = 0; i < inputs.size(); i++) {
        vector<double> output = forward(inputs[i]);
        loss += mse_loss(targets[i], output);
    }
    return loss / inputs.size();
}

//-----------------------
//Функція для виводу ваг
//-----------------------
void NeuralNetwork::print_weights() {
    for (size_t layer = 0; layer < weights.size(); ++layer) {
        cout << "Weights for layer " << layer << ":\n";
        for (size_t i = 0; i < weights[layer].size(); ++i) {
            for (size_t j = 0; j < weights[layer][i].size(); ++j) {
                cout << weights[layer][i][j] << " ";
            }
            cout << endl;
        }
    }
}


std::vector<std::vector<std::vector<double>>> NeuralNetwork::get_weights() {
    return weights;
}

void NeuralNetwork::set_weights(const std::vector<std::vector<std::vector<double>>>& new_weights) {
    //Перевіряємо, що розмір структур співпадає
    if (new_weights.size() != weights.size()) {
        std::cerr << "Error: wrong number of layers in weights file\n";
        return;
    }
    for (size_t l = 0; l < weights.size(); ++l) {
        if (new_weights[l].size() != weights[l].size()) {
            std::cerr << "Error: wrong number of neurons in layer " << l << "\n";
            return;
        }
    }
    weights = new_weights;
}