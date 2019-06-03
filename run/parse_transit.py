import pandas as pd
import numpy as np


def parse_shapeFile(filename):
    df = pd.read_csv(filename, index_col='shape_id')
    print(df.loc['41509'])


if __name__ == "__main__":
    parse_shapeFile("../resrc/capmetro/shapes.txt")
