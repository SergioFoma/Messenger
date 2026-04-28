import numpy as np
from machine_learning import neural_pred, get_pred_number
from file_func import read_weights

map = np.load('mnist.npz')
x_test = map['x_test']
y_test = map['y_test']

weights_0, weights_1, weights_2 = read_weights()

x_test = x_test.reshape(-1, 28 * 28)
matches = 0                                                                     # количество совпадений
for index in range(len(x_test)):

    input_data = (np.array(x_test[index])).reshape(1, -1)
    input_data = input_data / 255
    correct_answer = y_test[index]

    layer_0, layer_1, pred = neural_pred(input_data, weights_0, weights_1, weights_2)
    pred_number = get_pred_number(pred)

    if correct_answer == pred_number:
        matches += 1
    else:
        print("NEURAL WRONG PREDICT = ", pred_number, "\nCORRECT_VALUE = ", correct_answer, "\n")

print("NUMBER OF MATCHES = ", matches)
