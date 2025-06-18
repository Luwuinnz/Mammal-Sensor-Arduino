#Imports-----------------------------

from pathlib import Path
from typing import Dict, Tuple, List

import numpy as np
import pandas as pd
from scipy.signal import resample_poly, butter, filtfilt

import torch
import torch.nn as nn
from torch.utils.data import Dataset, DataLoader

from pytorch_tcn import TCN # Temporal Convolution Network
from ahrs.filters import Madgwick

#------------------------------------

#Ask if we want to filter noise and what Hz to resample to
class CleanPreprocessor:
    pass


# The DOGTCNMODEL
class DogTCNModel(nn.Module):
    def __init__(self, n_breeds: int, n_classes: int,
               tcn_channels: Tuple[int, ...] = (64,64,64,64),
               breed_emb: int = 16):
        #n_breeds - how many distinct dog breeds we want to implement
        #n_classes - how many diagnosis labels to predict
        #tcn_channels - the width of each Temporal-Conv Layer
        #breed_emb - size of the learnable vector that represents each breed
        
        #keeps track of parameters through parent call
        super().__init__()

        #to create the TCN with a 6-channel motion sequence
        self.tcn = TCN(input_size = 6,
                       output_size = tcn_channels[-1],
                       kernel_size = 3,
                       droupout = 0.1,
                       casual = False)
        #kernel_size is the maginfying glass (this frame, one before, and one after)
        #dropout, hides 10% of internal activations, prevent over-fitting(memorization vs leaning)
        #casual = False, means we can look at the frames after


        #creates a global average layer (avg presence of every motion motif across rec)
        self.pool = nn.AdaptiveAvgPool1d(1)

        #creates a dictionary of these ID entries
        #Pytorch makes a matrix of n_breeds (ID) x breed_emb (length of vector)
        self.breed_emb = nn.Embedding(n_breeds, breed_emb)

        #creates a learnable breed_emb dim vector
        self.meta_fc = nn.Sequential(
            nn.Linear(breed_emb + 2, 32), #16(breed) + 1(age) + 1(sex) = 18, scaled to 32
            nn.ReLU(), #changes neg # to 0s
            nn.Drouput(0.1) #prevents overfitting, by setting 10% to 0s
        )

        #CrossEntropyloss, concatenating the 64 motion features and 32 static
        self.final = nn.Sequential(
            nn.Linear(tcn_channels[-1] + 32, 64), #combines motion branch (64) + 32 static (breed-age-sex),
             # shrinked to 64
            nn.ReLU(),  #changes any neg numbers into zeros
            nn.Linear(64, n_classes) # final outputs vector without probability
        )

        #Forward pass
        def forward(self, seq: torch.Tensor, meta: Dict[str, torch.Tensor]):
            #outputs a 64-number fingerprint of the dog's motion
            ts = self.pool(self.tcn(seq)).squeeze(-1)
            #self.tcn(seq) runs the whole IMU data through TCN
            #self.pool() - average over time to get one vector per dog
            #.squeeze(-1) - drops length dimension

            #turn the dogbreed ID into learned embedding evctor
            #meta['breed'] holds each dog's breed ID
            #looks up the integer in the table and gives the vector from dict
            breed_vec = self.breed_emb(meta['breed'])

            #unsqueeze adds an additional dimension for concat
            #creates a 32-number summary for each dog based on breed, sex,age
            static = self.meta_fc(torch.cat([breed_vec,
                                             meta['sex'].unsqueeze(1).float(),
                                             meta['age'].unsqueeze(1)], dim=1))
            #run through mini-MLP to get a 32-number static feature vect
            fused = torch.cat([ts, static], dim =1)
            return self.final(fused)
