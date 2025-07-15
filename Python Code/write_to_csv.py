import csv

# Edit as needed
input_file = "E:/test.txt"      # Path to SD log
output_file = "cleaned_data.csv"  # New file

#change this
default_class = "unknown"  

# Indexes, adjust if needed

MPU_A_AccX_idx = 2
MPU_A_AccY_idx = 3
MPU_A_AccZ_idx = 4

MPU_B_AccX_idx = 14
MPU_B_AccY_idx = 15
MPU_B_AccZ_idx = 16

with open(input_file, "r") as infile, open(output_file, "w", newline='') as outfile:
    reader = csv.reader(infile)
    writer = csv.writer(outfile)

    # Write header for new CSV
    writer.writerow([
        "MPU_A_AccX", "MPU_A_AccY", "MPU_A_AccZ",
        "MPU_B_AccX", "MPU_B_AccY", "MPU_B_AccZ",
        "class"
    ])

    for i, row in enumerate(reader):
        # Skip rows that are too short
        if i == 0 and not row[0].replace('.', '', 1).isdigit():
            continue
        if len(row) < max(MPU_B_AccZ_idx+1, MPU_A_AccZ_idx+1):
            continue

        try:
            # Extract the six sensor values
            A_AccX = row[MPU_A_AccX_idx]
            A_AccY = row[MPU_A_AccY_idx]
            A_AccZ = row[MPU_A_AccZ_idx]
            B_AccX = row[MPU_B_AccX_idx]
            B_AccY = row[MPU_B_AccY_idx]
            B_AccZ = row[MPU_B_AccZ_idx]
        except IndexError:
            continue

        # change this later
        disease_label = default_class

        writer.writerow([A_AccX, A_AccY, A_AccZ, B_AccX, B_AccY, B_AccZ, disease_label])

print(f"Saved cleaned data to {output_file}")
