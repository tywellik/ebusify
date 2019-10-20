# -*- coding: utf-8 -*-

import pandas as pd

sources = {
    "Fayette Power Project 2 (steam cycle)":     {'type':'CoalPlant',    'MaxCap':285,    'MinCap':55.0, 'Running Cost': 4.33, 'Ramp Rate': 1.8},
    "South Texas Project 2 (steam cycle":        {'type':'NuclearPlant', 'MaxCap':200,    'MinCap':50.0, 'Running Cost': 2.30, 'Ramp Rate': 3.5},
    "South Texas Project 1 (steam cycle":        {'type':'NuclearPlant', 'MaxCap':200,    'MinCap':50.0, 'Running Cost': 2.30, 'Ramp Rate': 3.5},
    "Sand Hill GT 6 (simple cycle)":             {'type':'NatGasPlant',  'MaxCap':49,     'MinCap':45.0, 'Running Cost': 4.93, 'Ramp Rate': 10.0},
    "Decker GT 3 (simple cycle)":                {'type':'NatGasPlant',  'MaxCap':48,     'MinCap':45.0, 'Running Cost': 4.93, 'Ramp Rate': 10.0},
    "Decker 1 (steam cycle)":                    {'type':'NatGasPlant',  'MaxCap':323,    'MinCap':45.0, 'Running Cost': 4.93, 'Ramp Rate': 3.0},
    "Sand Hill GT 3 (simple cycle)":             {'type':'NatGasPlant',  'MaxCap':49,     'MinCap':45.0, 'Running Cost': 4.93, 'Ramp Rate': 10.0},
    "Sand Hill 5C (steam) (combined cycle)":     {'type':'NatGasPlant',  'MaxCap':154,    'MinCap':45.0, 'Running Cost': 4.93, 'Ramp Rate': 3.0},
    "Sand Hill GT 2 (simple cycle)":             {'type':'NatGasPlant',  'MaxCap':49,     'MinCap':45.0, 'Running Cost': 4.93, 'Ramp Rate': 10.0},
    "Sand Hill GT 4 (simple cycle)":             {'type':'NatGasPlant',  'MaxCap':49,     'MinCap':45.0, 'Running Cost': 4.93, 'Ramp Rate': 10.0},
    "Decker 2 (steam cycle)":                    {'type':'NatGasPlant',  'MaxCap':435,    'MinCap':45.0, 'Running Cost': 4.93, 'Ramp Rate': 3.0},
    "Fayette Power Project 1 (steam cycle)":     {'type':'CoalPlant',    'MaxCap':285,    'MinCap':55.0, 'Running Cost': 4.33, 'Ramp Rate': 1.8},
    "Sand Hill 5A (gas) (combined cycle)":       {'type':'NatGasPlant',  'MaxCap':162,    'MinCap':45.0, 'Running Cost': 4.93, 'Ramp Rate': 3.0},
    "Sand Hill GT 1 (simple cycle)":             {'type':'NatGasPlant',  'MaxCap':49,     'MinCap':45.0, 'Running Cost': 4.93, 'Ramp Rate': 10.0},
    "Decker GT 2 (simple cycle)":                {'type':'NatGasPlant',  'MaxCap':48,     'MinCap':45.0, 'Running Cost': 4.93, 'Ramp Rate': 10.0},
    "Decker GT 1 (simple cycle)":                {'type':'NatGasPlant',  'MaxCap':48,     'MinCap':45.0, 'Running Cost': 4.93, 'Ramp Rate': 10.0},
    "Decker GT 4 (simple cycle)":                {'type':'NatGasPlant',  'MaxCap':48,     'MinCap':45.0, 'Running Cost': 4.93, 'Ramp Rate': 10.0},
    "Sand Hill GT 7 (simple cycle)":             {'type':'NatGasPlant',  'MaxCap':49,     'MinCap':45.0, 'Running Cost': 4.93, 'Ramp Rate': 10.0},
    "Wind (aggregated) - base case":             {'type':'Wind',         'MaxCap':1219,   'MinCap':0.0,  'Running Cost': 0.00, 'Ramp Rate': float('inf')},
    "Solar (aggregated) - base case":            {'type':'Solar',        'MaxCap':528.6,  'MinCap':0.0,  'Running Cost': 0.00, 'Ramp Rate': float('inf')},
}

df = pd.DataFrame()

for src in sources:
    ser = pd.Series([sources[src]['type'], sources[src]['MaxCap'], sources[src]['MinCap'], sources[src]['Running Cost'], sources[src]['Ramp Rate']])
    df = df.append(ser, ignore_index=True )

df.columns = ['Type', 'Max Cap', 'Min Cap', 'Running Cost', 'Ramp Rate']
ser = pd.Series(sources.keys())
df['Name'] = ser
df = df[['Name', 'Type', 'Max Cap', 'Min Cap', 'Running Cost', 'Ramp Rate']]
#ind = pd.Index(sources.keys())
#df.set_index(ind, drop=True, inplace=True)

df.to_csv('source_info.csv', index=False)
print(df)