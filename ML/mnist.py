import numpy as np
import time
from machine_learning import machine_learn, get_pred_number
from file_func import save_weights, read_weights

# Сначала подготовим данные для нейронной сети: получение матриц и преобразования в нужный размер
map = np.load('mnist.npz')
x_train = map['x_train']                                           # 60 тысяч тренировочних данных ( 28 * 28 )
y_train = map['y_train']                                           # 60 тясяч ответов ( числа )

x_train = x_train.reshape(-1, 28 * 28)                             # матрица 28 * 28 --> массив 1 * 784

# для промежутка [a, b) формула (b-a) * random() + a
# я генерирую от [-0.1, 0.1)
np.random.seed()                                                   # при каждом запуске значения одинаковые
weights_0 = 0.2 * np.random.random((784, 400)) - 0.1
weights_1 = 0.2 * np.random.random((400, 100)) - 0.1
weights_2 = 0.2 * np.random.random((100,10)) - 0.1

number_of_epochs = 3
for epoch in range(number_of_epochs):
    for index in range(len(x_train)):

        goal_pred = [0] * 10
        correct_answer = y_train[index]                            # пусть правильный ответ 3, тогда:
        goal_pred[correct_answer] = 1                              # [0, 0, 0, 1, 0, 0, .... ]

        input_data = (np.array(x_train[index])).reshape(1, -1)     # двумерный массив 1 * 784
        input_data = input_data / 255;
        goal_pred = np.array(goal_pred)

        error, pred = machine_learn(input_data, weights_0, weights_1, weights_2, goal_pred)
        pred_number = get_pred_number(pred)
        print("NEURAL PREDICT = ", pred_number, "\ngoal_pred = ", correct_answer, "\nERROR = ", error)

    print("EPOCHS COUNT = ", epoch + 1)
    time.sleep(5)                                                   # 5 секунд показываем надпись о прохождении эпохи

save_weights(weights_0, weights_1, weights_2)                       # загрузка весов для последующего использоваия

"""
* Градиент (goal_pred - result ) * input_data.
* Если input_data принимает большое значение, то и градиент принимает большое значение, то есть обучение идет дольше
* Выполним нормировку: [0, 255] --> [0, 1]
"""
