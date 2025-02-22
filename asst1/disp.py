import numpy as np
import matplotlib.pyplot as plt

def plot_line_chart(A):
    l = len(A)
    color = ['blue', 'red', 'green', 'yellow', 'black', 'purple', 'orange', 'pink']
    for index in range(l):
        Ax = A[index]
        plt.plot(Ax, color=color[index], label=f'Method {index}')
        
    plt.legend()
    plt.xlabel('Number of cores')
    plt.ylabel('Speedup')
    plt.xticks(np.arange(len(A[0])), [8, 7, 6, 5, 4, 3, 2, 1])
    # plt.xticks(np.arange(len(A[0])), np.arange(1, len(A[0]) + 1))
    plt.title('Speedup vs Number of cores')
    plt.grid(True)
    plt.savefig('Speedup.png')


def read_speedup_file(file_path):
    with open(file_path, 'r') as file:
        lines = file.readlines()
        return np.array([float(line.strip().split(',')[1]) for line in lines])
# file_path = '/home/xm/parallel/asst1/prog1_mandelbrot_threads/speedup_0.txt'
# A0 = read_speedup_file(file_path)
# file_path = '/home/xm/parallel/asst1/prog1_mandelbrot_threads/speedup_1.txt'
# A1 = read_speedup_file(file_path)
# file_path = '/home/xm/parallel/asst1/prog1_mandelbrot_threads/speedup_2.txt'
# A2 = read_speedup_file(file_path)

A = [1.29, 1.31, 1.35, 1.41, 1.50, 1.26, 1.10, 1]
plot_line_chart([A])