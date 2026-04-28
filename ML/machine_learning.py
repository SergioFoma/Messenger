import numpy as np
from math import exp

def relu(layer):
    return np.maximum(layer, 0)

def softmax(last_layer_row):

    max_elem = max(last_layer_row)
    if not np.isfinite(max_elem):
        print("ERROR: max elem is inf")
    for i in range(len(last_layer_row)):
        last_layer_row[i] = exp(last_layer_row[i] - max_elem)
        if not np.isfinite(last_layer_row[i]):
            print("ERROR: last_layer_row[i] is inf")

    sum_elements = sum(last_layer_row)
    for i in range(len(last_layer_row)):
        last_layer_row[i] = last_layer_row[i] / sum_elements

    return last_layer_row

def softmax_wrap(last_layer):                                           # матрица
    for i in range(len(last_layer)):
        last_layer[i] = softmax(last_layer[i])

    return last_layer

def neural_pred(input_data, weights_0, weights_1, weights_2):

    layer_0 = relu(np.dot(input_data, weights_0))                            # применяем функцию активации relu
    layer_1 = relu(np.dot(layer_0, weights_1))                          # применяем функцию активации relu
    output = softmax_wrap(np.dot(layer_1, weights_2))                   # numbers predict (0-9) + функция активации softmax

    return layer_0, layer_1, output

def machine_learn(input_data, weights_0, weights_1, weights_2, goal_pred):

    alpha = 0.001                                                       # для корректировки веса
    #print( "Error = ", output_error )
    # нахождение предполагаемого значения и расчет ошибки
    layer_0, layer_1, pred = neural_pred(input_data, weights_0, weights_1, weights_2)
    output_error = np.mean((pred - goal_pred)**2)                   # среднее арифметическое квадратов разностей

    # обратное распределение
    output_delta = pred - goal_pred                                 # находим разность между ожидаемыми цифрами и текущими
    layer_1_delta = output_delta.dot(weights_2.T)                   # находим изменения значений в скрытом слое по изменениям в следующем слое
    layer_0_delta = layer_1_delta.dot(weights_1.T)

    # изменение весов
    weights_2 -= alpha * np.dot(layer_1.T, output_delta)
    weights_1 -= alpha * np.dot(layer_0.T, layer_1_delta)
    weights_0 -= alpha * np.dot(input_data.T, layer_0_delta)

    return output_error, pred

def get_pred_number(pred):

    neural_pred = pred[0]
    max_el = min(neural_pred)
    max_el_index = 0
    for el_index in range(len(neural_pred)):
        if neural_pred[el_index] > max_el:
            max_el = neural_pred[el_index]
            max_el_index = el_index

    return max_el_index
