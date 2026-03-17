import os

def pack_binary(input_file, output_dir, chunk_size=1024*1024*10): # 10MB chunks
    if not os.path.exists(output_dir):
        os.makedirs(output_dir)
    
    if not os.path.exists(input_file):
        print(f"Error: {input_file} not found.")
        return

    file_size = os.path.getsize(input_file)
    print(f"Packing {input_file} ({file_size} bytes)...")

    xor_key = 0xAB # Simple XOR key for demonstration

    with open(input_file, 'rb') as f:
        chunk_idx = 0
        while True:
            data = f.read(chunk_size)
            if not data:
                break
            
            # Apply XOR encryption
            encrypted_data = bytes([b ^ xor_key for b in data])
            
            output_filename = os.path.join(output_dir, f"data.{chunk_idx:03d}")
            with open(output_filename, 'wb') as out_f:
                out_f.write(encrypted_data)
            
            print(f"  Created {output_filename}")
            chunk_idx += 1

    print("Packing complete.")

if __name__ == "__main__":
    # Adjust paths as needed
    pack_binary("f:/aws-nuke/new_doom/external/aws-nuke.exe", "f:/aws-nuke/cpp_doom/data")
