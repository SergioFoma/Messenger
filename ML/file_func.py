import numpy as np

file_name = "weights.npz"

def save_weights(weights_0, weights_1, weights_2):

    np.savez(file_name, zero_weights = weights_0, first_weights = weights_1, second_weights = weights_2)

def read_weights():

    all_weights = np.load(file_name)
    weights_0 = all_weights['zero_weights']
    weights_1 = all_weights['first_weights']
    weights_2 = all_weights['second_weights']

    return weights_0, weights_1, weights_2
