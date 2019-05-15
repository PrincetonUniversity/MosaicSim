#!/bin/python

# Copyright 2017 Columbia University, SLD Group

import sys
import random

def gen(d1, d2, d3, name):

    matrix1 = open(name + "_1.txt", 'w')

    matrix1.write("2 ")           # dim
    matrix1.write(str(d1) + " ")  # d1
    matrix1.write(str(d2) + "\n") # d2

    for i in range(0, d1 * d2):
        matrix1.write(str(random.uniform(-5, 5)) + " ")

    matrix1.close()

    matrix2 = open(name + "_2.txt", 'w')

    matrix2.write("2 ")           # dim
    matrix2.write(str(d3) + " ")  # d1
    matrix2.write(str(d2) + "\n") # d2

    for i in range(0, d2 * d3):
        matrix2.write(str(random.uniform(-5, 5)) + " ")

    matrix2.close()

def main():

    gen(4,   4,   4,   "test1")
    gen(8,   16,  8,   "test2")
    gen(32,  64,  32,  "test3")
    gen(64,  64,  64,  "test4")
    gen(128, 256, 128, "test5")

    print("Info: input successfully generated")

if __name__ == "__main__":
    main()

