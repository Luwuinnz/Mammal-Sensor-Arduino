import pandas as pd
import numpy as np
import torch

# Example preprocess function (simplified for demonstration)
def preprocess_imu_csv(path, target_hz=50, cutoff=10, fixed_len=1000):
    df = pd.read_csv(path)
    # Extract IMU data (shape: [channels, N])
    imu_data = df[['acc_x', 'acc_y', 'acc_z', 'gyro_x', 'gyro_y', 'gyro_z']].values.T

    # (Optional) Resampling, filtering, padding, etc. would go here
    # For this demo, we just crop/pad to fixed_len columns
    if imu_data.shape[1] >= fixed_len:
        imu_final = imu_data[:, :fixed_len]
    else:
        pad_width = fixed_len - imu_data.shape[1]
        imu_final = np.pad(imu_data, ((0,0),(0,pad_width)), 'constant')
    return imu_final

# Load the data
file_path = 'dog01.csv'
imu_arr = preprocess_imu_csv(file_path, fixed_len=1000)
seq = torch.tensor(imu_arr, dtype=torch.float32).unsqueeze(0)  # [1, 6, 1000]

# Extract meta and label from the first row (since they're repeated)
df = pd.read_csv(file_path)
meta = {
    'breed': torch.tensor([int(df['breed'].iloc[0])], dtype=torch.long),
    'sex': torch.tensor([int(df['sex'].iloc[0])], dtype=torch.long),
    'age': torch.tensor([float(df['age'].iloc[0])], dtype=torch.float32)
}
disease_label = df['disease'].iloc[0]
print("Meta info:", meta)
print("Disease label:", disease_label)

# To map string label to integer class for training:
disease_to_idx = {'arthritis': 0, 'healthy': 1, 'hip_dysplasia': 2}
label_tensor = torch.tensor([disease_to_idx[disease_label]], dtype=torch.long)
print("Disease class index:", label_tensor)
