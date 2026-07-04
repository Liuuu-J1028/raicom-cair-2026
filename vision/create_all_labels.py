import os

def create_labels():
    """为所有图片创建标签文件"""
    train_images_dir = r'C:\Users\26630\Desktop\PythonProject\dataset\train\images'
    train_labels_dir = r'C:\Users\26630\Desktop\PythonProject\dataset\train\labels'
    valid_images_dir = r'C:\Users\26630\Desktop\PythonProject\dataset\valid\images'
    valid_labels_dir = r'C:\Users\26630\Desktop\PythonProject\dataset\valid\labels'

    os.makedirs(train_labels_dir, exist_ok=True)
    os.makedirs(valid_labels_dir, exist_ok=True)

    def process_images(images_dir, labels_dir, dataset_name):
        image_files = [f for f in os.listdir(images_dir) if f.lower().endswith('.jpg')]
        
        created_count = 0
        for img_file in image_files:
            base_name = os.path.splitext(img_file)[0]
            label_path = os.path.join(labels_dir, base_name + '.txt')
            
            if os.path.exists(label_path):
                continue
            
            # 根据文件名判断类别
            if '螺栓' in img_file:
                cls_id = 1  # bolt
            elif '齿轮' in img_file:
                cls_id = 0  # gear
            else:
                cls_id = 0  # 默认齿轮
            
            # 创建标签文件（假设物体在图片中心）
            with open(label_path, 'w') as f:
                f.write(f"{cls_id} 0.5 0.5 0.8 0.8\n")
            
            created_count += 1
            print(f"创建标签: {img_file} -> {base_name}.txt")
        
        print(f"\n{dataset_name}集：已创建 {created_count} 个标签文件")

    print("处理训练集...")
    process_images(train_images_dir, train_labels_dir, "训练")
    
    print("\n处理验证集...")
    process_images(valid_images_dir, valid_labels_dir, "验证")

    print("\n完成！")

if __name__ == '__main__':
    create_labels()
