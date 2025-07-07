import torch
from preprocessing import preprocess_imu_csv
from model import DogTCNModel

# Preprocess your test data
imu_arr = preprocess_imu_csv('dog01.csv', target_hz=50, cutoff=10, fixed_len=1000)
seq = torch.tensor(imu_arr, dtype=torch.float32).unsqueeze(0)

meta = {
    'breed': torch.tensor([2]), 
    'sex': torch.tensor([1]),
    'age': torch.tensor([6.0])
}

n_breeds = 20
n_classes = 4

model = DogTCNModel(n_breeds, n_classes)
model.eval()

with torch.no_grad():
    outputs = model(seq, meta)
    probs = torch.softmax(outputs, dim=1)
    predicted_class = probs.argmax(dim=1)

print("Raw outputs:", outputs)
print("Probabilities:", probs)
print("Predicted class:", predicted_class)
