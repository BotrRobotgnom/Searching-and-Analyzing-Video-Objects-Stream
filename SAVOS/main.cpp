#include "neural_network.h"
#include <iostream>
#include <algorithm>

using namespace std;

vector<vector<double>> inputs;
vector<vector<double>> targets;

void generate_data(int n) {
    inputs.clear();
    targets.clear();

    srand(time(0));

    for (int i = 0; i < n; i++) {
        int target = rand() % 2;
        vector<double> input(3);

        if (target == 1) {
            double num1 = rand() % 10 + 1;
            double num2 = rand() % 10 + 1;

            input = { num1, num1, num2 };
            random_shuffle(input.begin(), input.end());
        }
        else {
            double num1 = rand() % 10 + 1;
            double num2, num3;
            do { num2 = rand() % 10 + 1; } while (num2 == num1);
            do { num3 = rand() % 10 + 1; } while (num3 == num1 || num3 == num2);
            input = { num1, num2, num3 };
        }

        inputs.push_back(input);
        targets.push_back({ (double)target });
    }
}


int main() {
    /*
    int n = 10; // Кількість прикладів
    generate_data(n);

    // Вивід результатів
    cout << "Inputs:\n";
    for (size_t i = 0; i < inputs.size(); i++) {
        cout << "{ ";
        for (double num : inputs[i]) cout << num << " ";
        cout << "} -> Target: " << targets[i][0] << "\n";
    }
    */

    // Визначення структури нейромережі (кількість нейронів у кожному шарі)
    vector<int> layers = { 2, 2, 1 }; // 3 входи, 2 приховані шари (5 і 3 нейрони), 1 вихід
    double learning_rate = 0.02;
    cout << ">1" << endl;
    // Ініціалізація мережі
    NeuralNetwork nn(layers, learning_rate);
    cout << ">2" << endl;
    //Навчальні дані (вхідні значення та очікуваний вихід)
    /*vector<vector<double>> inputs = {
        {1}, {3}, {5}, {7}, {2}, {1}, {7}, {8}, {9}, {0}
    };
    vector<vector<double>> targets = {
        {0}, {0}, {1}, {1}, {0}, {0}, {1}, {1}, {1}, {0}
    };*/
    cout << ">3" << endl;

    //int n = 200;
    //generate_data(n);
    //Навчання мережі

    vector<vector<double>> inputs = {
    {0, 0},
    {0, 1},
    {1, 0},
    {1, 1}
    };

    vector<vector<double>> targets = {
        {0},
        {0},
        {0},
        {1}
    };

    for (int j = 0; j < 50; ++j)
    {
        
        cout << ">3" << endl;
        nn.train_batch(inputs, targets, 1000);
        nn.print_weights();

        //Перевірка мережі
        vector<vector<double>> test_inputs = {
           {0, 0}, {0, 1}, {1, 0}, {1, 1}
        };

        cout << "Test NN:" << endl;
        for (auto& test : test_inputs) {
            vector<double> output = nn.forward(test);
            cout << "Enter: (" << test[0] << test[1] << ") -> Exit: "
                << (output[0] > 0.5 ? "Yes" : "No") << endl;
        }
    }

    cout << ">4" << endl;
    return 0;
}
