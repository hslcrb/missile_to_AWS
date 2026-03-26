import os

def convert_to_cpp():
    if not os.path.exists("resource_types.txt"):
        print("resource_types.txt not found!")
        return
        
    encodings = ['utf-16', 'utf-8', 'cp949']
    types = None
    for enc in encodings:
        try:
            with open("resource_types.txt", "r", encoding=enc) as f:
                types = [line.strip() for line in f if line.strip()]
            print(f"Read using {enc}")
            break
        except Exception:
            continue
            
    if types is None:
        print("Failed to read file with known encodings!")
        return
    
    priorities = {"VPC", "S3Bucket", "EC2Instance", "RDSInstance", "IAMUser", "LambdaFunction"}
    
    p_items = sorted([t for t in types if t in priorities])
    other_items = sorted([t for t in types if t not in priorities])
    
    final_list = p_items + other_items
    
    # Generate C++ snippets
    with open("cpp_array.txt", "w", encoding="utf-8") as out:
        out.write("#ifndef RESOURCE_LIST_H\n#define RESOURCE_LIST_H\n\n")
        out.write("const wchar_t* g_fullResourceTypes[] = {\n")
        out.write('    L"<모두>",\n')
        for t in final_list:
            out.write(f'    L"{t}",\n')
        out.write("};\n")
        out.write("const int g_numResourceTypes = sizeof(g_fullResourceTypes) / sizeof(g_fullResourceTypes[0]);\n\n")
        out.write("#endif\n")
    
    print(f"Successfully generated cpp_array.txt with {len(final_list) + 1} items (Priority items first).")

if __name__ == "__main__":
    convert_to_cpp()
